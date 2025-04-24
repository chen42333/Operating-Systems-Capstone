#include "process.h"
#include "mem.h"
#include "syscall.h"

volatile pid_t last_pid = 0;
struct list ready_queue, dead_queue, wait_queue[_LAST];
struct pcb_t *pcb_table[MAX_PROC];

void init_pcb()
{
    struct pcb_t *pcb;

    pcb = calloc(1, sizeof(struct pcb_t));
    pcb->pid = 0;
    pcb->pc = idle;
    pcb->state = RUN;
    pcb->el = 1;
    pcb->pstate = 0x5; // EL1h (using SP1) with unmasked DAIF
    pcb->sp_el = (uint64_t)_estack;
    pcb->stack[1] = (uint8_t*)_estack;
    pcb->sp = (uintptr_t)_estack;
    asm volatile ("mov %0, x29" : "=r"(pcb->fp));
    pcb->lr = (uintptr_t)exit;

    pcb_table[0] = pcb;
    set_current(pcb);
}

void free_init_pcb()
{
    free((void*)pcb_table[0]);
}

pid_t thread_create(void (*func)(void *args), void *args)
{
    struct pcb_t *pcb;
    pid_t pid;

    for (pid = (last_pid + 1) % MAX_PROC; pcb_table[pid] != NULL; pid++)
    {
        if (pid == last_pid)
        {
            err("Cannot create more threads\r\n");
            return -1;
        }
    }

    pcb = calloc(1, sizeof(struct pcb_t));
    pcb->pid = pid;
    pcb->pc = func;
    pcb->args = args;
    pcb->state = READY;
    pcb->el = 1;
    pcb->pstate = 0x5; // EL1h (using SP1) with unmasked DAIF
    pcb->stack[0] = malloc(STACK_SIZE) + STACK_SIZE;
    pcb->stack[1] = malloc(STACK_SIZE) + STACK_SIZE;
    pcb->sp_el = (uint64_t)pcb->stack[1];
    pcb->sp = (uint64_t)pcb->stack[1];
    pcb->fp = (uint64_t)pcb->stack[1];
    pcb->lr = (uintptr_t)exit;

    pcb_table[pid] = pcb;
    last_pid = pid;

    list_push(&ready_queue, pcb);

    return pid;
}

void schedule()
{
    struct pcb_t *prev, *next;

    if (list_empty(&ready_queue))
        return;

    prev = get_current();
    prev->state = READY;
    list_push(&ready_queue, prev);

    prev->pc = &&out;

    next = list_pop(&ready_queue);
    next->state = RUN;
    set_current(next);

    // It is at EL1 currently, so it doesn't need to save/restore it additionally if prev->el == 1
    if (prev->el == 0)
        asm volatile("mrs %0, sp_el0" : "=r"(prev->sp_el));
    if (next->el == 0)
        asm volatile ("msr sp_el0, %0" :: "r"(next->sp_el));
    prev->pstate = 0x5; // EL1h (using SP1) with unmasked DAIF
    switch_to(prev->reg, next->reg, next->pc, next->pstate, next->args);

out:
    ; // the following will do the restoration of fp and lr
}

void _exit()
{
    register int syscall_id __asm__("x8") = 5;
    asm volatile ("svc #0":: "r"(syscall_id): "memory");
}

static void kill_zombies()
{
    struct pcb_t *pcb = list_pop(&dead_queue);

    while (pcb)
    {
        deref_code(pcb->code);
        if (pcb->stack[0])
            free(pcb->stack[0] - STACK_SIZE);
        free(pcb->stack[1] - STACK_SIZE);
        pcb_table[pcb->pid] = NULL;
        free(pcb);
        pcb = list_pop(&dead_queue);
    }
}

void idle()
{
    while (true)
    {
        kill_zombies();
        // schedule();
    }
}

void wait(event e, size_t data)
{
    struct pcb_t *pcb = get_current(), *next;
    struct wait_q_e *w = malloc(sizeof(struct wait_q_e));

    w->pcb = pcb;
    w->data = data;
    pcb->state = WAIT;
    pcb->wait_q = &wait_queue[e];
    list_push(&wait_queue[e], w);

    pcb->pc = &&out;

    if (list_empty(&ready_queue))
        return;

    next = list_pop(&ready_queue);
    next->state = RUN;
    set_current(next);

    // It is at EL1 currently, so it doesn't need to save/restore it additionally if prev->el == 1
    if (pcb->el == 0)
        asm volatile("mrs %0, sp_el0" : "=r"(pcb->sp_el));
    if (next->el == 0)
        asm volatile ("msr sp_el0, %0" :: "r"(next->sp_el));
    pcb->pstate = 0x5; // EL1h (using SP1) with unmasked DAIF
    switch_to(pcb->reg, next->reg, next->pc, next->pstate, next->args);

out:
    ; // the following will do the restoration of fp and lr
}

void wait_to_ready(void *ptr)
{
    struct wait_q_e *e = (struct wait_q_e*)ptr;
    struct pcb_t *pcb = e->pcb;
    
    free(e);
    pcb->state = READY;
    pcb->wait_q = NULL;
    list_push(&ready_queue, pcb);
}