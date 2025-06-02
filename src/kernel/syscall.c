#include "syscall.h"
#include "uart.h"
#include "mailbox.h"
#include "ramdisk.h"
#include "vmem.h"
#include "file.h"
#include "dev.h"

void syscall_entry(struct trap_frame *frame)
{
    enable_int();

    switch ((syscall)frame->nr_syscall)
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
        case SIGNAL:
            signal((int)frame->arg(0), (void (*)())frame->arg(1));
            break;
        case SIGNAL_KILL:
            signal_kill((int)frame->arg(0), (int)frame->arg(1));
            break;
        case MMAP:
            frame->RET = (size_t)mmap((void*)frame->arg(0), (size_t)frame->arg(1), (int)frame->arg(2), 
                                    (int)frame->arg(3), (int)frame->arg(4), (int)frame->arg(5));
            break;
        case OPEN:
            frame->RET = open((const char*)frame->arg(0), (int)frame->arg(1));
            break;
        case CLOSE:
            frame->RET = close((int)frame->arg(0));
            break;
        case WRITE:
            frame->RET = write((int)frame->arg(0), (const void*)frame->arg(1), (unsigned long)frame->arg(2));
            break;
        case READ:
            frame->RET = read((int)frame->arg(0), (void*)frame->arg(1), (unsigned long)frame->arg(2));
            break;
        case MKDIR:
            frame->RET = mkdir((const char*)frame->arg(0), (unsigned int)frame->arg(1));
            break;
        case MOUNT:
            frame->RET = mount((const char*)frame->arg(0), (const char*)frame->arg(1), 
                            (const char*)frame->arg(2), (unsigned long)frame->arg(3), (const void*)frame->arg(4));
            break;
        case CHDIR:
            frame->RET = chdir((const char*)frame->arg(0));
            break;
        case LSEEK:
            frame->RET = lseek64((int)frame->arg(0), (long)frame->arg(1), (int)frame->arg(2));
            break;
        case IOCTL:
            frame->RET = ioctl((int)frame->arg(0), (unsigned long)frame->arg(1), (void*)frame->arg(2));
            break;
        case SIGRET:
            sigreturn();
            break;
        default:
            err("Unknown syscall\r\n");
            break;
    }

    disable_int();
}

pid_t getpid()
{
    return get_current()->pid;
}

int exec(const char* name, char *const argv[])
{
    struct pcb_t *pcb = get_current();
    void *prog_addr;
    size_t prog_size;

    if (!(prog_addr = load_prog((char*)name, &prog_size)))
    {
        printf("File not found\r\n");
        exit();
        return -1;
    }

    disable_int();

    if (pcb->el == 0)
    {
        free_sections(&pcb->sections, pcb->ttbr);
        free_page_table(pcb->ttbr, PGD);
    }
    
    pcb->el = 0;
    pcb->pstate = EL0_W_DAIF;
    pcb->lr = (uintptr_t)_exit;
    pcb->code = v2p_trans_kernel(prog_addr);
    pcb->sp_el0 = (uint64_t)USR_STACK_END;

    init_ttbr(pcb);

    add_section(pcb, TEXT, USR_CODE_START, prog_size);
    add_section(pcb, STACK, USR_STACK_START, STACK_EL0_SIZE);
    
    enable_int();

    exec_prog(USR_CODE_START, USR_STACK_END, pcb->ttbr);

    return 0;
}

static bool match_pid(void *ptr, void *data)
{
    return (pid_t)((struct pcb_t*)ptr)->wait_data == *(pid_t*)data;
}

static void add_ref_sections(struct list *l, void *ttbr)
{
    struct node *tmp = l->head;

    while (tmp)
    {
        struct section *s = tmp->ptr;

        if (s->type != DEVICE)
        {
            for (size_t i = 0; i < s->size; i += PAGE_SIZE)
            {
                void *ptr = v2p_trans(s->base + i, ttbr);
                ref_page(ptr);
            }
        }

        tmp = tmp->next;
    }
}

int fork()
{
    struct pcb_t *pcb = get_current(), *new_pcb;
    void *frame_ptr, *stack_ptr, *tmp;;
    volatile pid_t new_pid;

    disable_int();

    new_pid = thread_create(&&out, pcb->args);

    new_pcb = pcb_table[new_pid];
    new_pcb->el = pcb->el;
    new_pcb->cur_dir = pcb->cur_dir;
    for (int i = 0; i < MAX_NUM_FD; i++)
    {
        if (pcb->fd_table[i])
        {
            new_pcb->fd_table[i] = pcb->fd_table[i];
            pcb->fd_table[i]->ref_count++;
        }
    }
    new_pcb->fd_bitmap = pcb->fd_bitmap;
    if (pcb->el == 0)
    {
        add_ref_sections(&pcb->sections, pcb->ttbr);
        new_pcb->code = pcb->code;
        list_copy(&new_pcb->sections, &pcb->sections);
        init_ttbr(new_pcb);
        page_table_fork(new_pcb->ttbr, pcb->ttbr, PGD);
        invalidate_tlb();
        asm volatile("mrs %0, sp_el0" : "=r"(new_pcb->sp_el0));
    }
    memcpy(new_pcb->stack[1] - STACK_EL1_SIZE, pcb->stack[1] - STACK_EL1_SIZE, STACK_EL1_SIZE);
    
    memcpy(new_pcb->sig_handler, pcb->sig_handler, sizeof(new_pcb->sig_handler));

    // Copy current regs to new_pcb
    asm volatile ("mov %0, fp": "=r"(frame_ptr));
    asm volatile ("mov %0, sp": "=r"(stack_ptr));
    frame_ptr = (void*)new_pcb->stack[1] - ((void*)pcb->stack[1] - frame_ptr);
    stack_ptr = (void*)new_pcb->stack[1] - ((void*)pcb->stack[1] - stack_ptr);

    tmp = frame_ptr;

    // Modify all the stored fp on the stack to the new value
    while (tmp < (void*)new_pcb->stack[1])
    {   
        void *update_fp = (void*)new_pcb->stack[1] - ((void*)pcb->stack[1] - *(void**)tmp);

        if (update_fp <= tmp || update_fp > (void*)new_pcb->stack[1]) // Reach the 1st frame
            break;

        *(void**)tmp = update_fp;
        tmp = *(void**)tmp;
    }

    save_regs(new_pcb->reg, frame_ptr, __builtin_return_address(0), stack_ptr);

    enable_int();

out:
    if (get_current()->pid != new_pid) // parent
        return new_pid;
    else // child
        return 0;
}

void exit()
{
    struct pcb_t *pcb = get_current();

    if (pcb->state == DEAD)
        return;

    disable_int();

    pcb->state = DEAD;
    list_push(&dead_queue, pcb);
    list_rm_and_process(&wait_queue[PROC], match_pid, &pcb->pid, wait_to_ready);

    enable_int();

    if (list_empty(&ready_queue))
        return;

    switch_to_next(pcb);
}

void kill(pid_t pid)
{
    struct pcb_t *pcb = pcb_table[pid];
    if (!pcb)
    {
        err("No such process\r\n");
        return;
    }

    disable_int();

    switch (pcb->state)
    {
        case RUN:
            enable_int();
            exit();
            break;
        case READY:
            list_delete(&ready_queue, (void*)pcb);
            pcb->state = DEAD;
            list_push(&dead_queue, (void*)pcb);
            list_rm_and_process(&wait_queue[PROC], match_pid, &pcb->pid, wait_to_ready);
            break;
        case WAIT:
            list_delete(pcb->wait_q, (void*)pcb);
            pcb->state = DEAD;
            list_push(&dead_queue, (void*)pcb);
            list_rm_and_process(&wait_queue[PROC], match_pid, &pcb->pid, wait_to_ready);
            break;
        case DEAD:
            enable_int();
            return;
        default:
            err("Unknown process state\r\n");
            break;
    }

    enable_int();
}