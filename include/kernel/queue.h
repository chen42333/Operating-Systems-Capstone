#ifndef __QUEUE_H
#define __QUEUE_H

#include "utils.h"
#include "mem.h"

struct q_element
{
    void *ptr;
    struct q_element *next;
};

struct queue
{
    struct q_element *head, *tail;
};

inline static bool process_queue_empty(struct queue *q)
{
    return !q->head;
}

inline static void process_queue_push(struct queue *q, void *ptr)
{
    struct q_element *tmp = malloc(sizeof(struct q_element));
    tmp->ptr = ptr;

    if (q->tail)
        q->tail->next = tmp;
    else
        q->head = tmp;
    q->tail = tmp;
}

inline static void* process_queue_pop(struct queue *q)
{
    struct q_element *head = q->head;
    void *ret = NULL;
    
    if (!process_queue_empty(q))
    {
        ret = head->ptr;
        q->head = q->head->next;
        free(head);
    }

    return ret;
}

#endif