#ifndef __RING_BUF_H
#define __RING_BUF_H

#define BUFLEN 256

#include "mem.h"

enum buf_type {
    CHAR,
    TIMER,
    TASK
};

struct timer_queue_element
{
    void (*handler)(void*);
    uint64_t cur_ticks;
    uint64_t duration_ticks;
    void *data;
};

struct task_queue_element
{
    void (*handler)(void*);
    uint32_t priority;
    void *data;
};

struct ring_buf
{
    int producer_idx;
    int consumer_idx;
    void *buf;
};

inline static void ring_buf_init(struct ring_buf *rb, enum buf_type type)
{
    rb->producer_idx = 0; // The next position to put
    rb->consumer_idx = 0; // The next position to read

    switch (type)
    {
        case CHAR:
            rb->buf = simple_malloc(sizeof(char) * BUFLEN);
            break;
        case TIMER:
            rb->buf = simple_malloc(sizeof(struct timer_queue_element) * BUFLEN);
            break;
        case TASK:
            rb->buf = simple_malloc(sizeof(struct task_queue_element) * BUFLEN);
            break;
        default:
            break;
    }
}

inline static bool ring_buf_full(struct ring_buf *rb)
{
    // Wasting 1 byte to differentiate empty/full
    return (rb->producer_idx + 1) % BUFLEN == rb->consumer_idx;
}

inline static bool ring_buf_empty(struct ring_buf *rb)
{
    return rb->producer_idx == rb->consumer_idx;
}

inline static void ring_buf_produce(struct ring_buf *rb, void *data, enum buf_type type)
{
    while (ring_buf_full(rb)) ;

    switch (type)
    {
        case CHAR:
            ((char*)rb->buf)[rb->producer_idx++] = *(char*)data;
            break;
        case TIMER:
            ((struct timer_queue_element*)rb->buf)[rb->producer_idx++] = *(struct timer_queue_element*)data;
            break;
        case TASK:
            ((struct task_queue_element*)rb->buf)[rb->producer_idx++] = *(struct task_queue_element*)data;
            break;
        default:
            break;
    }
    
    rb->producer_idx %= BUFLEN;
}

inline static void ring_buf_consume(struct ring_buf *rb, void *ret, enum buf_type type)
{
    while (ring_buf_empty(rb)) ;

    switch (type)
    {
        case CHAR:
            *(char*)ret = ((char*)rb->buf)[rb->consumer_idx++];
            break;
        case TIMER:
            *(struct timer_queue_element*)ret = ((struct timer_queue_element*)rb->buf)[rb->consumer_idx++];
            break;
        case TASK:
            *(struct task_queue_element*)ret = ((struct task_queue_element*)rb->buf)[rb->consumer_idx++];
            break;
        default:
            break;
    }

    rb->consumer_idx %= BUFLEN;
}

#endif