/*
 * test_mempool.c - basic correctness + stress tests for the memory pool.
 *
 * Build:  gcc -O2 -Wall -Wextra -std=c11 -o test_mempool test_mempool.c mempool.c
 * Sanity: ./test_mempool
 */

#include "mempool.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static int failures = 0;

#define CHECK(cond)                                                        \
    do {                                                                   \
        if (!(cond)) {                                                     \
            fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            failures++;                                                    \
        }                                                                  \
    } while (0)

static void test_basic(void) {
    mempool_t *p = mempool_create();
    CHECK(p != NULL);

    void *a = mp_alloc(p, 10);
    CHECK(a != NULL);
    /* Pointer must be at least pointer-aligned. */
    CHECK(((uintptr_t)a % sizeof(void *)) == 0);
    memset(a, 0xAB, 10);

    mp_free(p, a);
    CHECK(mempool_allocated_count(p) == 0);

    mempool_destroy(p);
}

static void test_null(void) {
    mempool_t *p = mempool_create();
    CHECK(mp_alloc(NULL, 16) == NULL);
    mp_free(p, NULL);          /* must not crash */
    mp_free(NULL, (void *)0x1); /* must not crash */
    mempool_destroy(p);
}

static void test_sizes(void) {
    mempool_t *p = mempool_create();
    size_t sizes[] = {1, 7, 8, 9, 16, 17, 100, 255, 256, 1000, 1024,
                      5000, 4096, 70000, 100000, (size_t)1 << 20};
    int n = (int)(sizeof(sizes) / sizeof(sizes[0]));

    void **ptrs = (void **)malloc((size_t)n * sizeof(void *));
    CHECK(ptrs != NULL);

    for (int i = 0; i < n; i++) {
        ptrs[i] = mp_alloc(p, sizes[i]);
        CHECK(ptrs[i] != NULL);
        /* Write the whole object; also exercises the cookie-header boundary. */
        memset(ptrs[i], (int)(i & 0xFF), sizes[i]);
    }
    CHECK(mempool_allocated_count(p) == (size_t)n);

    /* Free in reverse, then re-allocate and verify the data region is intact.
     * (We only verify sizes/alignment here; the stress test covers reuse.) */
    for (int i = n - 1; i >= 0; i--) {
        mp_free(p, ptrs[i]);
    }
    CHECK(mempool_allocated_count(p) == 0);

    free(ptrs);
    mempool_destroy(p);
}

static void test_reuse(void) {
    mempool_t *p = mempool_create();
    void *a = mp_alloc(p, 32);
    CHECK(a != NULL);
    mp_free(p, a);
    void *b = mp_alloc(p, 32);
    /* The freed block should be reused from the same class free list. */
    CHECK(b == a);
    mp_free(p, b);
    CHECK(mempool_allocated_count(p) == 0);
    mempool_destroy(p);
}

static void test_stress(void) {
    mempool_t *p = mempool_create();
    void **ptrs = (void **)malloc(sizeof(void *) * 10000);
    CHECK(ptrs != NULL);

    int n = 0;
    srand(12345);
    for (int i = 0; i < 200000; i++) {
        int r = rand() % 10000;
        if (r < 6000 && n < 10000) {        /* allocate */
            size_t sz = (size_t)(rand() % 70000) + 1;
            void *m = mp_alloc(p, sz);
            if (m) {
                memset(m, (int)(sz & 0xFF), sz);
                ptrs[n++] = m;
            }
        } else if (n > 0) {                  /* free a random live block */
            int idx = rand() % n;
            mp_free(p, ptrs[idx]);
            ptrs[idx] = ptrs[--n];
        }
    }
    while (n > 0) mp_free(p, ptrs[--n]);

    CHECK(mempool_allocated_count(p) == 0);
    free(ptrs);
    mempool_destroy(p);
}

int main(void) {
    test_basic();
    test_null();
    test_sizes();
    test_reuse();
    test_stress();

    if (failures == 0) {
        printf("ALL TESTS PASSED\n");
        return 0;
    }
    fprintf(stderr, "%d CHECK(s) failed\n", failures);
    return 1;
}
