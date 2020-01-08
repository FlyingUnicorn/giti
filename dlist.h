#ifndef DLIST_H_
#define DLIST_H_

#include <stdlib.h>

typedef struct dlist dlist_t;

dlist_t*
dlist_create(void);

void
dlist_append(dlist_t* dl, void* data);

void
dlist_foreach(const dlist_t* dl, void (*dlist_foreach_fn)(void* data, void* arg), void* arg);

void
dlist_destroy(dlist_t* dl, void (*dlist_free_fn)(void* data));

size_t
dlist_size(const dlist_t* dl);

void*
dlist_get(const dlist_t* dl, size_t indx);

#endif
