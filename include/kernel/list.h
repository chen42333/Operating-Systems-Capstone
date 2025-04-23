#ifndef __LIST_H
#define __LIST_H

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

inline static void* list_top(struct list *l)
{
    return l->head->ptr;
}

void list_push(struct list *l, void *ptr);
void* list_pop(struct list *l);
void list_delete(struct list *l, void *ptr);
void* list_find(struct list *l, bool (*match)(void *ptr));
void list_rm_and_process(struct list *l, bool (*match)(void *ptr, void *data), void *match_data, void (*op)(void *ptr));

#endif