#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include <stdint.h>
#include "ring_buf.h"
#include "utils.h"

#define CORE0_TIMER_IRQ_CTRL (void*)(0x40000040 + v_kernel_space)
#define TIMER_INT 10 // ms

// Bit 31~26 of ESR
#define EC_MASK 0xfc000000
#define EC_DATA_ABORT 0b100100
#define EC_INSTRUCTION_ABORT 0b100000
// Bit 5~2 of ESR
#define DFSC_MASK 0x3c
#define DFSC_ADDR_SIZE 0b0000
#define DFSC_TRANSLATION 0b0001
#define DFSC_ACC_FLAG 0b0010
#define DFSC_PERMISSION 0b0011
// Bit 1~0 of ESR
#define LEVEL_MASK 0x3

extern struct ring_buf task_queue;
extern struct task_queue_element task_queue_buf[BUFLEN];
extern volatile int irq_nested_count;

typedef enum prio {
    WRITE_PRIO = 0,
    TIMER_PRIO = 1,
    READ_PRIO = 2,
    INIT_PRIO = INT_MAX
} prio;

void add_timer(void(*callback)(void*), uint64_t duration, void *data);
void add_task(void(*callback)(void*), prio priority, void *data);
void timer_int();
void process_timer(void *data);
void elasped_time(void *data);
void print_msg(void *data);
void init_timer_queue();
void exception_entry();
void process_task(struct task_queue_element *task);
void tx_int_task(void *data);
void tx_int();
void rx_int_task(void *data);
void rx_int();
int set_timeout();
void core_timer_enable();

inline static void enable_timer_int()
{
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"((uint64_t)1));
}

inline static void disable_timer_int()
{
    asm volatile ("msr cntp_ctl_el0, %0" :: "r"((uint64_t)0));
}

inline static void task_queue_init()
{
    ring_buf_init(&task_queue, task_queue_buf);
}

inline static void disable_int() 
{
    asm volatile("msr daifset, #0xf" ::: "memory");
    irq_nested_count++;

    asm volatile("dmb ish" ::: "memory"); // Ensure all memory accesses after here see the locked state
}

inline static void enable_int() 
{
    asm volatile("dmb ish" ::: "memory"); // Ensure all memory writes before releasing lock

    if (--irq_nested_count == 0)
        asm volatile("msr daifclr, #0xf" ::: "memory");
}

#endif