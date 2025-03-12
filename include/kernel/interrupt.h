#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include <stdint.h>
#include "ring_buf.h"

#define CORE0_TIMER_IRQ_CTRL (void*)0x40000040
#define TIMER_QUEUE_LEN 128

struct t_q 
{
    int producer_idx;
    int consumer_idx;
    struct timer_queue_element *buf;
};

extern struct t_q timer_queue;

void add_timer(void(*callback)(void*), uint64_t duration, void *data);
int set_timeout();
void core_timer_enable();

#endif