#include "list.h"
#include "utils.h"

void list_push(struct list *l, void *ptr)
{
    struct node *tmp = calloc(1, sizeof(struct node));
    tmp->ptr = ptr;

    if (l->tail)
    {
        l->tail->next = tmp;
        tmp->prev = l->tail;
    }
    else
        l->head = tmp;
    l->tail = tmp;
}

void* list_pop(struct list *l)
{
    struct node *head = l->head;
    void *ret = NULL;
    
    if (!list_empty(l))
    {
        ret = head->ptr;
        l->head = l->head->next;
        if (l->head)
            l->head->prev = NULL;
        else
            l->tail = NULL;
        free(head);
    }

    return ret;
}

void list_delete(struct list *l, void *ptr)
{
    struct node *tmp = l->head;

    while (tmp)
    {
        if (tmp->ptr == ptr)
        {
            if (tmp->prev)
                tmp->prev->next = tmp->next;
            else
                l->head = tmp->next;

            if (tmp->next)
                tmp->next->prev = tmp->prev;
            else
                l->tail = tmp->prev;

            free(tmp);
            return;
        }

        tmp = tmp->next;
    }

    err("Node not found\r\n");
}

void* list_find(struct list *l, bool (*match)(void *ptr))
{
    struct node *tmp = l->head;

    while (tmp)
    {
        if (match(tmp->ptr))
            return tmp->ptr;

        tmp = tmp->next;
    }

    return NULL;
}

void list_rm_and_process(struct list *l, bool (*match)(void *ptr, void *data), void *match_data, void (*op)(void *ptr))
{
    struct node *tmp = l->head;

    while (tmp)
    {
        struct node *next = tmp->next;

        if (match(tmp->ptr, match_data))
        {
            op(tmp->ptr);
            if (tmp->prev)
                tmp->prev->next = tmp->next;
            else
                l->head = tmp->next;

            if (tmp->next)
                tmp->next->prev = tmp->prev;
            else
                l->tail = tmp->prev;

            free(tmp);
        }

        tmp = next;
    }
}