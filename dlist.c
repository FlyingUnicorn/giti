
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

static void*
dlist_pop(dlist_t* dl)
{
    if (!dl->size) {
        return NULL;
    }

    dlist_entry_t* e = dl->head->prev;
    void* data = e->data;

    e->prev->next = e->next;
    e->next->prev = e->prev;

    free(e);

   --dl->size;
   return data;
}

void
dlist_foreach(const dlist_t* dl, void (*dlist_foreach_fn)(void* data, void* arg), void* arg)
{
    dlist_entry_t* e = dl->head;
    for (int i = 0; i < dl->size; ++i) {
        dlist_foreach_fn(e->data, arg);
        e = e->next;
    }
}

void
dlist_clear(dlist_t* dl, void (*dlist_free_fn)(void* data))
{
    void* data;
    while((data = dlist_pop(dl))) {
        if (dlist_free_fn) {
            dlist_free_fn(data);
        }
    }
}

void
dlist_destroy(dlist_t* dl, void (*dlist_free_fn)(void* data))
{
    dlist_clear(dl, dlist_free_fn);
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

    it->head = dl->size ? dl->head : NULL;
    it->last = NULL;

    return it;
}

void*
dlist_iterator_next(dlist_iterator_t* it)
{
    void* data = NULL;
    if (it->head == NULL) {
    }
    else if (it->last == NULL) {
        it->last = it->head;
        data = it->last->data;
    }
    else if (it->last->next != it->head) {
        it->last = it->last->next;
        data = it->last->data;
    }

    return data;
}

void
dlist_iterator_destroy(dlist_iterator_t* it)
{
    free(it);
}
