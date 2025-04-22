#include "syscall.h"
#include "uart.h"
#include "mailbox.h"
#include "ramdisk.h"

void syscall_entry(struct trap_frame *frame)
{
    switch ((enum syscall)frame->nr_syscall)
    {
        case GET_PID:
            frame->RET = (size_t)getpid();
            break;
        case UART_READ:
            frame->RET = uart_read((char*)frame->arg(0), frame->arg(1));
            break;
        case UART_WRITE:
            frame->RET = uart_write((const char*)frame->arg(0), frame->arg(1));
            break;
        case EXEC:
            frame->RET = (size_t)exec((const char*)frame->arg(0), (char *const*)frame->arg(1));
            break;
        case FORK:
            frame->RET = (size_t)fork();
            break;
        case EXIT:
            exit();
            break;
        case MBOX_CALL:
            frame->RET = (size_t)mbox_call((unsigned char)frame->arg(0), (unsigned int*)frame->arg(1));
            break;
        case KILL:
            kill((pid_t)frame->arg(0));
            break;
        default:
            err("Unknown syscall\r\n");
            break;
    }
}

pid_t getpid()
{
    return get_current()->pid;
}

int exec(const char* name, char *const argv[])
{
    struct pcb_t *pcb = get_current();

    if (load_prog((char*)name) < 0)
        return -1;
    
    pcb->el = 0;
    pcb->pstate = 0x0; // EL0 with unmasked DAIF
    exec_prog(prog_addr, pcb->stack + STACK_SIZE);

    return 0;
}

int fork()
{
    struct pcb_t *pcb = get_current(), *new_pcb;
    void *frame_ptr, *stack_ptr;
    volatile pid_t new_pid;

    new_pid = thread_create(&&out, pcb->args);

    new_pcb = pcb_table[new_pid];
    memcpy(new_pcb->stack - STACK_SIZE, pcb->stack - STACK_SIZE, STACK_SIZE);
    
    // Copy current regs to new_pcb
    asm volatile ("mov %0, fp": "=r"(frame_ptr));
    asm volatile ("mov %0, sp": "=r"(stack_ptr));
    frame_ptr = (void*)new_pcb->stack - ((void*)pcb->stack - frame_ptr);
    stack_ptr = (void*)new_pcb->stack - ((void*)pcb->stack - stack_ptr);
    save_regs(new_pcb->reg, frame_ptr, __builtin_return_address(0), stack_ptr);

    // Modify all the stored fp on the stack to the new value
    while (frame_ptr < (void*)new_pcb->stack)
    {
        *(void**)frame_ptr = (void*)new_pcb->stack - ((void*)pcb->stack - *(void**)frame_ptr);
        frame_ptr = *(void**)frame_ptr;
    }

out:
    if (get_current()->pid != new_pid) // parent
        return new_pid;
    else // child
        return 0;
}

void exit()
{
    struct pcb_t *pcb = get_current(), *next;

    if (pcb->state == DEAD)
        return;

    pcb->state = DEAD;
    list_push(&dead_queue, pcb);

    if (list_empty(&ready_queue))
        return;

    next = list_pop(&ready_queue);
    next->state = RUN;
    set_current(next);

    // It is at EL1 currently, so it doesn't need to save/restore it additionally if prev->el == 1
    if (next->el == 0)
        asm volatile ("msr sp_el0, %0" :: "r"(next->sp_el));
    switch_to(pcb->reg, next->reg, next->pc, next->pstate, next->args);
}

void kill(pid_t pid)
{
    switch (pcb_table[pid]->state)
    {
        case RUN:
            exit();
            break;
        case READY:
            list_delete(&ready_queue, pcb_table[pid]);
            pcb_table[pid]->state = DEAD;
            list_push(&dead_queue, pcb_table[pid]);
            break;
        case WAIT:
            list_delete(&wait_queue, pcb_table[pid]);
            pcb_table[pid]->state = DEAD;
            list_push(&dead_queue, pcb_table[pid]);
            break;
        case DEAD:
            return;
        default:
            err("Unknown process state\r\n");
            break;
    }
}