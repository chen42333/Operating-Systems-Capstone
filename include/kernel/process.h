#ifndef __PROCESS_H
#define __PROCESS_H

#include <stdint.h>
#include "utils.h"

#define MAX_PROC 1024
#define STACK_SIZE 4096
#define NR_CALLEE_REGS 13 // x19 ~ x31, where x29 = fp, x30 = lr, x31 = sp
#define fp reg[29 - 19]
#define lr reg[30 - 19]
#define sp reg[31 - 19]

typedef int pid_t;

typedef enum stat { RUN, READY, WAIT, DEAD } stat;

struct pcb_t
{
    pid_t pid;
    uint64_t reg[NR_CALLEE_REGS];
    void *pc;
    uint8_t stack[STACK_SIZE];
    stat state;
    // uint64_t pstate;
    struct pcb_t *next;
};

struct process_queue
{
    struct pcb_t *head;
    struct pcb_t *tail;
};

inline static bool process_queue_empty(struct process_queue *q)
{
    return !q->head;
}

inline static void process_queue_push(struct process_queue *q, struct pcb_t *pcb)
{
    if (q->tail)
        q->tail->next = pcb;
    else
        q->head = pcb;
    q->tail = pcb;
}

inline static struct pcb_t* process_queue_pop(struct process_queue *q)
{
    struct pcb_t *ret = q->head;
    
    if (!process_queue_empty(q))
        q->head = q->head->next;

    return ret;
}

extern struct pcb_t* get_current();
extern void set_current(struct pcb_t *pcb);
extern void switch_to(uint64_t *prev_reg, uint64_t *next_reg, void *next_pc);
void init_pcb();
void free_init_pcb();
void thread_create(void (*func)());
void schedule();
void idle();
void exit();

#endif