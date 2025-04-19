#include "list.h"

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
        l->head->prev = NULL;
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

