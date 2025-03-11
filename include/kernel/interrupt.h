#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include <stdint.h>

#define TIMER_QUEUE_LEN 128

struct timer_queue_element
{
    void (*handler)(void*);
    uint64_t cur_ticks;
    uint64_t duration_ticks;
    void *data;
};

struct t_q 
{
    int producer_idx;
    int consumer_idx;
    struct timer_queue_element *buf;
};

extern struct t_q timer_queue;

void add_timer(void(*callback)(void*), uint64_t duration, void *data);

#endif