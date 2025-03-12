#ifndef __INTERRUPT_H
#define __INTERRUPT_H

#include <stdint.h>
#include "ring_buf.h"

#define CORE0_TIMER_IRQ_CTRL (void*)0x40000040

extern struct ring_buf timer_queue;

void add_timer(void(*callback)(void*), uint64_t duration, void *data);
int set_timeout();
void core_timer_enable();

#endif