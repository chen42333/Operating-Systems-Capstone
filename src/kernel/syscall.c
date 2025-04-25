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
            frame->RET = (size_t)fork(frame);
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
        case SIGNAL:
            signal((int)frame->arg(0), (void (*)())frame->arg(1));
            break;
        case SIGNAL_KILL:
            signal_kill((int)frame->arg(0), (int)frame->arg(1));
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
    void *prog_addr;

    if (!(prog_addr = load_prog((char*)name)))
    {
        printf("File not found\r\n");
        return -1;
    }
    
    pcb->el = 0;
    pcb->pstate = 0x0; // EL0 with unmasked DAIF
    pcb->lr = (uintptr_t)_exit;
    if (pcb->code)
        deref_code(pcb->code);
    pcb->code = init_code(prog_addr);
    pcb->sp_el0 = (uint64_t)pcb->stack[0];
    exec_prog(prog_addr, pcb->stack[0]);

    return 0;
}

int fork(struct trap_frame *frame)
{
    struct pcb_t *pcb = get_current(), *new_pcb;
    void *frame_ptr, *stack_ptr;
    volatile pid_t new_pid;

    new_pid = thread_create(&&out, pcb->args);

    new_pcb = pcb_table[new_pid];
    if (pcb->stack[0])
        memcpy(new_pcb->stack[0] - STACK_SIZE, pcb->stack[0] - STACK_SIZE, STACK_SIZE);
    memcpy(new_pcb->stack[1] - STACK_SIZE, pcb->stack[1] - STACK_SIZE, STACK_SIZE);
    new_pcb->el = pcb->el;
    new_pcb->code = ref_code(pcb->code);
    memcpy(new_pcb->sig_handler, pcb->sig_handler, sizeof(new_pcb->sig_handler));
    
    // Copy current regs to new_pcb
    asm volatile ("mov %0, fp": "=r"(frame_ptr));
    asm volatile ("mov %0, sp": "=r"(stack_ptr));
    frame_ptr = (void*)new_pcb->stack[1] - ((void*)pcb->stack[1] - frame_ptr);
    stack_ptr = (void*)new_pcb->stack[1] - ((void*)pcb->stack[1] - stack_ptr);
    save_regs(new_pcb->reg, frame_ptr, __builtin_return_address(0), stack_ptr);

    // Modify all the stored fp on the stack to the new value
    while (frame_ptr < (void*)new_pcb->stack[1])
    {
        *(void**)frame_ptr = (void*)new_pcb->stack[1] - ((void*)pcb->stack[1] - *(void**)frame_ptr);
        frame_ptr = *(void**)frame_ptr;
    }
    if (pcb->el == 0) // The fork is called by a user process, which has EL0 stack and trapframe
    {
        // Modify fp at the trap frame of child
        uint64_t el0_sp;
        struct trap_frame *new_frame = (void*)new_pcb->stack[1] - ((void*)pcb->stack[1] - (void*)frame);

        new_frame->x(29) = (size_t)((void*)new_pcb->stack[0] - ((void*)pcb->stack[0] - (void*)frame->x(29)));
        asm volatile("mrs %0, sp_el0" : "=r"(el0_sp));
        new_pcb->sp_el0 = (uint64_t)((void*)new_pcb->stack[0] - ((void*)pcb->stack[0] - (void*)el0_sp));

        frame_ptr = (void*)new_pcb->stack[0] - ((void*)pcb->stack[0] - (void*)frame->x(29)); // The trapframe stores register value in EL0, and PCB stores that in EL1
        while (frame_ptr < (void*)new_pcb->stack[0])
        {
            *(void**)frame_ptr = (void*)new_pcb->stack[0] - ((void*)pcb->stack[0] - *(void**)frame_ptr);
            frame_ptr = *(void**)frame_ptr;
        }
    }

out:
    if (get_current()->pid != new_pid) // parent
        return new_pid;
    else // child
        return 0;
}

static bool match_pid(void *ptr, void *data)
{
    return (pid_t)((struct pcb_t*)ptr)->wait_data == *(pid_t*)data;
}

void exit()
{
    struct pcb_t *pcb = get_current(), *next;

    if (pcb->state == DEAD)
        return;

    pcb->state = DEAD;
    list_push(&dead_queue, pcb);
    list_rm_and_process(&wait_queue[PROC], match_pid, &pcb->pid, wait_to_ready);

    if (list_empty(&ready_queue))
        return;

    switch_to_next(pcb);
}

void kill(pid_t pid)
{
    if (!pcb_table[pid])
    {
        err("No such process\r\n");
        return;
    }

    switch (pcb_table[pid]->state)
    {
        case RUN:
            exit();
            break;
        case READY:
            list_delete(&ready_queue, (void*)pcb_table[pid]);
            pcb_table[pid]->state = DEAD;
            list_push(&dead_queue, (void*)pcb_table[pid]);
            break;
        case WAIT:
            list_delete(pcb_table[pid]->wait_q, (void*)pcb_table[pid]);
            pcb_table[pid]->state = DEAD;
            list_push(&dead_queue, (void*)pcb_table[pid]);
            break;
        case DEAD:
            return;
        default:
            err("Unknown process state\r\n");
            break;
    }
}