#ifndef DLIST_H_
#define DLIST_H_

#include <stdlib.h>

typedef struct dlist dlist_t;
typedef struct dlist_iterator dlist_iterator_t;
dlist_t*
dlist_create(void);

void
dlist_append(dlist_t* dl, void* data);

void
dlist_foreach(const dlist_t* dl, void (*dlist_foreach_fn)(void* data, void* arg), void* arg);

void
dlist_clear(dlist_t* dl, void (*dlist_free_fn)(void* data));

void
dlist_destroy(dlist_t* dl, void (*dlist_free_fn)(void* data));

size_t
dlist_size(const dlist_t* dl);

void*
dlist_get(const dlist_t* dl, size_t indx);

void*
dlist_next(const dlist_t* dl, void* data);


dlist_iterator_t*
dlist_iterator_create(const dlist_t* dl);

void*
dlist_iterator_next(dlist_iterator_t* it);

void
dlist_iterator_destroy(dlist_iterator_t* it);

#endif
