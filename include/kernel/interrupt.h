#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include <stdint.h>
#include "ring_buf.h"
#include "utils.h"

#define CORE0_TIMER_IRQ_CTRL (void*)0x40000040
#define TIMER_INT 10 // ms

extern struct ring_buf task_queue;
extern struct task_queue_element task_queue_buf[BUFLEN];
extern volatile int irq_nested_count;

enum prio {
    TIMER_PRIO = 0,
    WRITE_PRIO = 1,
    READ_PRIO = 2,
    INIT_PRIO = INT_MAX
};

void add_timer(void(*callback)(void*), uint64_t duration, void *data);
void add_task(void(*callback)(void*), enum prio priority, void *data);
void timer_int();
void process_timer(void* data);
void elasped_time(void* data);
void print_msg(void *data);
void init_timer_queue();
void exception_entry();
void process_task(struct task_queue_element* task);
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