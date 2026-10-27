#ifndef MEMPOOL_H
#define MEMPOOL_H

#include <stddef.h>

/*
 * A small "toy" memory pool inspired by the allocator write-up in readmd.txt.
 *
 * Strategy (article step 3): a set of fixed-size free-list allocators, one per
 * size class (8, 16, 32, ... up to 64 KiB). Allocation looks up the smallest
 * class that fits, pops a free object, and returns it. Each object carries a
 * header "cookie" (the owning class) just before the user pointer so that
 * mp_free() knows which free list to return it to. Objects larger than the
 * biggest class are handed straight to the system malloc.
 *
 * This is intentionally simple: single-threaded, and class memory is only
 * returned to the OS when the whole pool is destroyed (like memcached's slab).
 */

typedef struct mempool mempool_t;

/* Create a memory pool. Returns NULL on OOM. */
mempool_t *mempool_create(void);

/* Destroy a pool, releasing all backing system memory. */
void mempool_destroy(mempool_t *pool);

/* Allocate `size` bytes. Returns NULL on failure. The returned pointer is
 * aligned to at least alignof(void *). */
void *mp_alloc(mempool_t *pool, size_t size);

/* Release memory previously returned by mp_alloc(). */
void mp_free(mempool_t *pool, void *ptr);

/* Number of currently live user allocations in the pool. */
size_t mempool_allocated_count(mempool_t *pool);

#endif /* MEMPOOL_H */
