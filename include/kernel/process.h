#ifndef __PROCESS_H
#define __PROCESS_H

#include <stdint.h>
#include "utils.h"
#include "list.h"
#include "signal.h"

#define MAX_PROC 1024
#define STACK_EL0_SIZE (1ULL << 14)
#define STACK_EL1_SIZE (1ULL << 13)
#define NR_CALLEE_REGS 13 // x19 ~ x31, where x29 = fp, x30 = lr, x31 = sp
#define fp reg[29 - 19]
#define lr reg[30 - 19]
#define sp reg[31 - 19]

#define EL0_W_DAIF 0x0 // EL0 with unmasked DAIF
#define EL1H_W_DAIF 0x5 // EL1h (using SP1) with unmasked DAIF

// Virtual memory address for user processes
#define USR_CODE_START (void*)0x0
#define USR_STACK_START (void*)0xffffffffb000
#define USR_STACK_END (void*)0xfffffffff000
#define USR_FRAMEBUF_START (void*)0x30000000

typedef int pid_t;

typedef enum stat { RUN, READY, WAIT, DEAD } stat;
typedef enum event { PROC, READ, WRITE, _LAST } event;
typedef enum sec { TEXT, HEAP, STACK, DEVICE } sec;

struct pcb_t
{
    pid_t pid;
    void *args;
    int el;
    uint64_t reg[NR_CALLEE_REGS]; // The sp is for EL1
    uint64_t reg_backup[NR_CALLEE_REGS]; // store/restore reg to/from reg_backup before/after running signal handler
    uint64_t sp_el0; // The sp for EL0 if (el == 0)
    uint64_t sp_el0_backup;
    void *pc;
    uint8_t *stack[2]; // For EL0 and EL1
    stat state;
    struct list *wait_q;
    uint64_t pstate;
    void *code; // Physical address
    size_t wait_data;
    void (*sig_handler[_NSIG])();
    struct list signal_queue;
    struct list sections;
    void *ttbr;
};

struct section
{
    sec type;
    void *base; // Virtual address
    size_t size;
};

extern struct list ready_queue, dead_queue, wait_queue[_LAST];
extern struct pcb_t *pcb_table[MAX_PROC];

extern struct pcb_t* get_current();
extern void set_current(struct pcb_t *pcb);
extern void switch_to(uint64_t *prev_reg, uint64_t *next_reg, void *next_pc, uint64_t next_pstat, void *next_args, void *next_ttbr);
extern void save_regs(void *addr, void *frame_ptr, void *ret_addr, void *stack_ptr);
extern void load_regs(uint64_t *next_reg, void *next_pc, uint64_t next_pstat, void *next_args, void *next_ttbr);

void ps();
void init_pcb();
void free_init_pcb();
pid_t thread_create(void (*func)(void *args), void *args);
void switch_to_next(struct pcb_t *prev);
void schedule();
void idle();
void _exit();
void add_section(struct pcb_t *pcb, sec type, void *base, size_t len);
void free_sections(struct list *sections, void *ttbr);
bool in_section(void *ptr, void *addr);
void wait(event e, size_t data);
void wait_to_ready(void *ptr);

#endif