#include "list.h"
#include "utils.h"

void list_push(struct list *l, void *ptr) {
    disable_int();

    struct node *tmp = calloc(1, sizeof(struct node));
    tmp->ptr = ptr;

    if (l->tail) {
        l->tail->next = tmp;
        tmp->prev = l->tail;
    }
    else
        l->head = tmp;
    l->tail = tmp;

    enable_int();
}

void* list_pop(struct list *l) {
    disable_int();

    struct node *head = l->head;
    void *ret = NULL;
    
    if (!list_empty(l)) {
        ret = head->ptr;
        l->head = l->head->next;
        if (l->head)
            l->head->prev = NULL;
        else
            l->tail = NULL;
        free(head);
    }

    enable_int();

    return ret;
}

void* list_delete(struct list *l, void *ptr) {
    disable_int();

    struct node *tmp = l->head;
    void *ret = NULL;

    while (tmp) {
        if (tmp->ptr == ptr) {
            if (tmp->prev)
                tmp->prev->next = tmp->next;
            else
                l->head = tmp->next;

            if (tmp->next)
                tmp->next->prev = tmp->prev;
            else
                l->tail = tmp->prev;

            ret = tmp->ptr;
            free(tmp);
            break;
        }

        tmp = tmp->next;
    }

    enable_int();

    if (!ret) {
        err("Node not found\r\n");
    }

    return ret;
}

void* list_find(struct list *l, bool (*match)(void *ptr, void *data), void *match_data) {
    disable_int();

    struct node *tmp = l->head;
    void *ret = NULL;

    while (tmp) {
        if (match(tmp->ptr, match_data)) {
            ret = tmp->ptr;
            break;
        }
        tmp = tmp->next;
    }

    enable_int();

    return ret;
}

void list_rm_and_process(struct list *l, bool (*match)(void *ptr, void *data), void *match_data, void (*op)(void *ptr)) {
    disable_int();

    struct node *tmp = l->head;

    while (tmp) {
        struct node *next = tmp->next;

        if (match(tmp->ptr, match_data)) {
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

    enable_int();
}

void list_copy(struct list *dst, struct list *src) {
    disable_int();

    struct node *tmp = src->head;

    while (tmp) {
        list_push(dst, tmp->ptr);
        tmp = tmp->next;
    }

    enable_int();
}