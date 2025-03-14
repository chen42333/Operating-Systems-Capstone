#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include <stdint.h>
#include "ring_buf.h"
#include "utils.h"

#define CORE0_TIMER_IRQ_CTRL (void*)0x40000040

extern struct ring_buf task_queue;

enum prio {
    TIMER_PRIO = 0,
    READ_PRIO = 1,
    WRITE_PRIO = 2,
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
    ring_buf_init(&task_queue, TASK);
}

#endif