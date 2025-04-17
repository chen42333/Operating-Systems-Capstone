#include "process.h"
#include "mem.h"

pid_t last_pid = 0;
struct process_queue ready_queue, dead_queue;
struct pcb_t *pcb_table[MAX_PROC];

void init_pcb()
{
    struct pcb_t *pcb;

    pcb = calloc(1, sizeof(struct pcb_t));
    pcb->pid = 0;
    pcb->pc = idle;
    pcb->state = RUN;
    pcb->next = NULL;
    pcb->sp = (uintptr_t)_estack;
    asm volatile ("mov %0, x29" : "=r"(pcb->fp));
    pcb->lr = (uintptr_t)exit;

    pcb_table[0] = pcb;
    set_current(pcb);
}

void free_init_pcb()
{
    free(pcb_table[0]);
}

void thread_create(void (*func)())
{
    struct pcb_t *pcb;
    pid_t pid;

    for (pid = (last_pid + 1) % MAX_PROC; pcb_table[pid] != NULL; pid++)
    {
        if (pid == last_pid)
        {
            err("Cannot create more threads\r\n");
            return;
        }
    }

    pcb = calloc(1, sizeof(struct pcb_t));
    pcb->pid = pid;
    pcb->pc = func;
    pcb->state = READY;
    pcb->next = NULL;
    pcb->sp = (uintptr_t)&pcb->stack[STACK_SIZE];
    pcb->fp = (uintptr_t)&pcb->stack[STACK_SIZE];
    pcb->lr = (uintptr_t)exit;

    pcb_table[pid] = pcb;
    last_pid = pid;

    process_queue_push(&ready_queue, pcb);
}

void schedule()
{
    struct pcb_t *prev, *next;

    if (process_queue_empty(&ready_queue))
        return;

    prev = get_current();
    prev->state = READY;
    process_queue_push(&ready_queue, prev);

    prev->pc = &&out;

    next = process_queue_pop(&ready_queue);
    next->state = RUN;
    set_current(next);

    switch_to(prev->reg, next->reg, next->pc);

out:
    ; // the following will do the restoration of caller-saved registers and return
}

void exit()
{
    struct pcb_t *pcb = get_current();

    pcb->state = DEAD;
    process_queue_push(&dead_queue, pcb);
}

static void kill_zombies()
{
    struct pcb_t *pcb = process_queue_pop(&dead_queue);

    while (pcb)
    {
        pcb_table[pcb->pid] = NULL;
        free(pcb);
        pcb = process_queue_pop(&dead_queue);
    }
}

void idle()
{
    while (true)
    {
        kill_zombies();
        schedule();
    }
}