#include "signal.h"
#include "process.h"
#include "syscall.h"

void signal(int signo, void (*handler)())
{
    struct pcb_t *pcb = get_current();

    pcb->sig_handler[signo] = handler;
}

void signal_kill(int pid, int signo)
{
    struct pcb_t *pcb  = pcb_table[pid];
    int *signo_ptr;

    if (!pcb)
    {
        err("No such process\r\n");
        return;
    }

    signo_ptr = malloc(sizeof(int));
    *signo_ptr = signo;
    list_push(&pcb->signal_queue, signo_ptr);
}

void _sigreturn()
{
    register int syscall_id __asm__("x8") = SIGRET;
    asm volatile ("svc #0":: "r"(syscall_id): "memory");
}

void sigreturn()
{
    struct pcb_t *pcb = get_current();

    memcpy(pcb->reg, pcb->reg_backup, sizeof(pcb->reg));
    pcb->sp_el0 = pcb->sp_el0_backup;
    asm volatile ("msr sp_el0, %0" :: "r"(pcb->sp_el0));
    
    load_regs(pcb->reg, pcb->pc, pcb->pstate, pcb->args, pcb->ttbr);
}