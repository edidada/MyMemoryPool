# MyMemoryPool

A small "toy" C memory pool, written as an exercise inspired by the allocator
write-up in `readmd.txt` (which walks through building allocators from a simple
free-list up to a real slab allocator).

## What it does

This project implements the free-list pool with size classes idea (article
step 3):

- A set of fixed-size free-list allocators, one per size class:
  `8, 16, 32, 64, 128, 256, 512, 1K, 2K, 4K, 8K, 16K, 32K, 64K` bytes.
- Allocation picks the smallest class that fits, pops a free object, and returns
  it. When a class is empty it grows by requesting one big block from the system
  and carving it into many objects.
- Every object carries a header cookie (the owning class) just before the
  user pointer, so `mp_free()` knows exactly which free list to return it to -
  no separate lookup table needed. Pattern: `ptr[-sizeof(header)]`.
- Objects larger than 64 KiB bypass the pool and go straight to `malloc`/`free`.
- Memory handed out by the classes is kept and recycled; it is only returned to
  the OS when the whole pool is destroyed (the same trade-off memcached's slab
  makes).

It is intentionally simple and single-threaded, exactly as the article suggests
for a "write it for fun" project.

## Build & test

```sh
make            # build the test binary with gcc
make run        # run the test suite
```

Or with clang:

```sh
make CC=clang run
```

Sanitizer build (AddressSanitizer + UBSan):

```sh
gcc -O1 -g -fsanitize=address,undefined -std=c11 -Wall -Wextra \
    -o test_asan test_mempool.c mempool.c && ./test_asan
```

## API

```c
mempool_t *mempool_create(void);
void       mempool_destroy(mempool_t *pool);
void      *mp_alloc(mempool_t *pool, size_t size);
void       mp_free(mempool_t *pool, void *ptr);
size_t     mempool_allocated_count(mempool_t *pool);
```

## Layout

| File            | Purpose                                         |
|-----------------|-------------------------------------------------|
| `mempool.h`     | Public API and design notes.                    |
| `mempool.c`     | Implementation (free-list pool + cookie header). |
| `test_mempool.c`| Correctness and stress tests.                   |
| `Makefile`      | Build rules.                                    |
| `.github/workflows/ci.yml` | CI: build + test on gcc/clang, plus sanitizers. |
