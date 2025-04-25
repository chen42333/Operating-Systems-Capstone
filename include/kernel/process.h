#ifndef __PROCESS_H
#define __PROCESS_H

#include <stdint.h>
#include "utils.h"
#include "list.h"

#define MAX_PROC 1024
#define STACK_SIZE 4096
#define NR_CALLEE_REGS 13 // x19 ~ x31, where x29 = fp, x30 = lr, x31 = sp
#define fp reg[29 - 19]
#define lr reg[30 - 19]
#define sp reg[31 - 19]

typedef int pid_t;

typedef enum stat { RUN, READY, WAIT, DEAD } stat;
typedef enum event { PROC, READ, WRITE, _LAST } event;

struct pcb_t
{
    pid_t pid;
    void *args;
    int el;
    uint64_t reg[NR_CALLEE_REGS];
    uint64_t sp_el0;
    void *pc;
    uint8_t *stack[2]; // For EL0 and EL1
    stat state;
    struct list *wait_q;
    uint64_t pstate;
    struct code_ref *code;
    size_t wait_data;
};

struct code_ref
{
    void *code;
    int ref;
};

extern struct list ready_queue, dead_queue, wait_queue[_LAST];
extern struct pcb_t *pcb_table[MAX_PROC];

extern struct pcb_t* get_current();
extern void set_current(struct pcb_t *pcb);
extern void switch_to(uint64_t *prev_reg, uint64_t *next_reg, void *next_pc, uint64_t next_pstat, void *next_args);
void init_pcb();
void free_init_pcb();
pid_t thread_create(void (*func)(void *args), void *args);
void schedule();
void idle();
void _exit();
void wait(event e, size_t data);
void wait_to_ready(void *ptr);

inline static struct code_ref* init_code(void *code)
{
    struct code_ref *ret = malloc(sizeof(struct code_ref));

    ret->code = code;
    ret->ref = 1;

    return ret;
}

inline static struct code_ref* ref_code(struct code_ref *code)
{
    code->ref++;
    return code;
}

inline static struct code_ref* deref_code(struct code_ref *code)
{
    if (--code->ref == 0)
    {
        free(code->code);
        free(code);
    }

    return NULL;
}

#endif