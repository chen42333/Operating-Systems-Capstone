#include "ring_buf.h"
#include "interrupt.h"

void ring_buf_produce(struct ring_buf *rb, void *data, enum buf_type type)
{    
    while (ring_buf_full(rb)) ;

    disable_int();

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

    enable_int();
}

void ring_buf_consume(struct ring_buf *rb, void *ret, enum buf_type type)
{
    while (ring_buf_empty(rb)) ;

    disable_int();

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

    enable_int();
}