/*
 * mempool.c - a simple free-list memory pool.
 *
 * See mempool.h for the design overview. Internally each allocation is one
 * contiguous block:
 *
 *     +------------------+-----------------------------+
 *     | chunk_header_t   | user data ...               |
 *     +------------------+-----------------------------+
 *     ^                  ^
 *     h = ptr - 1        ptr returned to the caller
 *
 * The header stores a "cookie" (owner) so mp_free() can route the block back
 * to the correct free list without any extra bookkeeping tables.
 */

#include "mempool.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Size classes: powers of two from 8 bytes up to 64 KiB. */
static const size_t CLASS_SIZES[] = {
    8, 16, 32, 64, 128, 256, 512, 1024,
    2048, 4096, 8192, 16384, 32768, 65536
};
#define NUM_CLASSES (int)(sizeof(CLASS_SIZES) / sizeof(CLASS_SIZES[0]))
#define MAX_OBJ     (CLASS_SIZES[NUM_CLASSES - 1])
#define OBJS_PER_BLOCK 64

/* Sentinel stored in the header's owner field for allocations that bypass the
 * pool and go straight to the system malloc (size > MAX_OBJ). */
static const int RAW_SENTINEL = 0;

typedef struct chunk_header {
    struct chunk_header *next; /* links the free list while the block is free */
    const void *owner;         /* cookie: mp_class_t* or &RAW_SENTINEL        */
    size_t size;               /* requested user size                          */
} chunk_header_t;

typedef struct mp_class {
    size_t obj_size;           /* user object size for this class              */
    size_t chunk_size;         /* sizeof(chunk_header_t) + obj_size            */
    chunk_header_t *free_list; /* free objects of this class                   */
    int objs_per_block;        /* objects carved out of one system allocation   */
} mp_class_t;

typedef struct block_node {
    void *ptr;                 /* base of a system allocation                  */
    struct block_node *next;
} block_node_t;

struct mempool {
    mp_class_t classes[NUM_CLASSES];
    block_node_t *blocks;      /* system blocks backing the classes            */
    size_t live;               /* number of live user allocations              */
};

mempool_t *mempool_create(void) {
    mempool_t *p = (mempool_t *)malloc(sizeof(mempool_t));
    if (!p) return NULL;
    memset(p, 0, sizeof(*p));
    for (int i = 0; i < NUM_CLASSES; i++) {
        p->classes[i].obj_size = CLASS_SIZES[i];
        p->classes[i].chunk_size = sizeof(chunk_header_t) + CLASS_SIZES[i];
        p->classes[i].free_list = NULL;
        p->classes[i].objs_per_block = OBJS_PER_BLOCK;
    }
    return p;
}

static void add_block(mempool_t *p, void *ptr) {
    block_node_t *n = (block_node_t *)malloc(sizeof(block_node_t));
    if (!n) return; /* best effort; we are already out of memory */
    n->ptr = ptr;
    n->next = p->blocks;
    p->blocks = n;
}

/* Find the smallest class whose object size is >= `size`. */
static int class_index(size_t size) {
    for (int i = 0; i < NUM_CLASSES; i++)
        if (CLASS_SIZES[i] >= size) return i;
    return -1;
}

/* Carve OBJS_PER_BLOCK objects out of one system allocation and push them onto
 * the class free list. */
static void grow(mempool_t *p, mp_class_t *c) {
    size_t block_bytes = c->chunk_size * (size_t)c->objs_per_block;
    char *block = (char *)malloc(block_bytes);
    if (!block) return;
    add_block(p, block);
    for (int i = 0; i < c->objs_per_block; i++) {
        chunk_header_t *h = (chunk_header_t *)(block + (size_t)i * c->chunk_size);
        h->owner = c;
        h->size = c->obj_size;
        h->next = c->free_list;
        c->free_list = h;
    }
}

void *mp_alloc(mempool_t *pool, size_t size) {
    if (!pool) return NULL;
    if (size == 0) size = 1;

    if (size > MAX_OBJ) {
        /* Too large for any class: hand off to the system allocator. */
        chunk_header_t *h = (chunk_header_t *)malloc(sizeof(chunk_header_t) + size);
        if (!h) return NULL;
        h->owner = &RAW_SENTINEL;
        h->size = size;
        h->next = NULL;
        pool->live++;
        return (void *)(h + 1);
    }

    int idx = class_index(size);
    mp_class_t *c = &pool->classes[idx];
    if (!c->free_list) grow(pool, c);
    if (!c->free_list) return NULL;

    chunk_header_t *h = c->free_list;
    c->free_list = h->next;
    h->owner = c;
    h->size = size;
    pool->live++;
    return (void *)(h + 1);
}

void mp_free(mempool_t *pool, void *ptr) {
    if (!pool || !ptr) return;
    chunk_header_t *h = (chunk_header_t *)ptr - 1;

    if (h->owner == (const void *)&RAW_SENTINEL) {
        /* Direct system allocation: release it immediately. */
        pool->live--;
        free(h);
        return;
    }

    mp_class_t *c = (mp_class_t *)h->owner;
    h->next = c->free_list;
    c->free_list = h;
    pool->live--;
}

size_t mempool_allocated_count(mempool_t *pool) {
    return pool ? pool->live : 0;
}

void mempool_destroy(mempool_t *pool) {
    if (!pool) return;
    block_node_t *b = pool->blocks;
    while (b) {
        block_node_t *next = b->next;
        free(b->ptr);
        free(b);
        b = next;
    }
    free(pool);
}
