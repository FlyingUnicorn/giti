
#include "dlist.h"

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>

// https://radek.io/2012/11/10/magical-container_of-macro/
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

typedef struct dlist_entry {
    struct dlist_entry* prev;
    struct dlist_entry* next;
    void*               data;
} dlist_entry_t;

struct dlist {
    size_t         size;
    dlist_entry_t* head;
};

static dlist_entry_t*
dlist_entry_create(void* data)
{
    dlist_entry_t* e = malloc(sizeof *e);
    e->prev = NULL;
    e->next = NULL;
    e->data = data;
    return e;
}

dlist_t*
dlist_create(void)
{
    dlist_t* dl = malloc(sizeof *dl);
    dl->size = 0;

    return dl;
}

void
dlist_append(dlist_t* dl, void* data)
{
    //printf("[%s]\n", __func__);
    dlist_entry_t* e = dlist_entry_create(data);
    if (dl->size) {
        e->prev = dl->head->prev;
        e->next = dl->head;
        dl->head->prev->next = e;
        dl->head->prev = e;
    }
    else {
        e->prev = e;
        e->next = e;
        dl->head = e;
    }
    ++dl->size;
}

void
dlist_foreach(const dlist_t* dl, void (*dlist_foreach_fn)(void* data, void* arg), void* arg)
{
    dlist_entry_t* e = dl->head;
    for (int i = 0; i < dl->size; ++i) {
        dlist_foreach_fn(e->data, arg);
        e = e->next;

        //if (e->next == dl->head) {
        //    break;
        //}
    }
}

void
dlist_destroy(dlist_t* dl, void (*dlist_free_fn)(void* data))
{
    //dlist_entry_t* e = dl->head;
    //while(e != NULL) {
    // dlist_free_fn(e->data);
        //dlist_entry_t* e_ = e;

    //  e->
        
    //  free(e_);
    //}
    free(dl);
}

size_t
dlist_size(const dlist_t* dl)
{
    return dl ? dl->size : 0;
}

void*
dlist_get(const dlist_t* dl, size_t indx)
{
    if (indx >= dl->size) {
        return NULL;
    }

    dlist_entry_t* e = dl->head;
    for (; indx > 0; --indx) {
        e = e->next;
    }
    return e->data;
}


struct dlist_iterator {
    dlist_entry_t* head;
    dlist_entry_t* last;
};

dlist_iterator_t*
dlist_iterator_create(const dlist_t* dl)
{
    dlist_iterator_t* it = malloc(sizeof *it);
    it->head = dl->head;
    it->last = NULL;

    return it;
}

void*
dlist_iterator_next(dlist_iterator_t* it)
{
    void* data = NULL;
    if (it->last == NULL) {
        it->last = it->head;
        data = it->last->data;
    }
    else if (it->last->next != it->head) {
        it->last = it->last->next;
        data = it->last->data;
    }

    return data;
}
