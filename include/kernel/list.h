#ifndef __LIST_H
#define __LIST_H

#include "utils.h"
#include "mem.h"

struct node
{
    void *ptr;
    struct node *prev;
    struct node *next;
};

struct list
{
    struct node *head, *tail;
};

inline static bool list_empty(struct list *l)
{
    return !l->head;
}

void list_push(struct list *l, void *ptr);
void* list_pop(struct list *l);
void list_delete(struct list *l, void *ptr);

#endif