#include "mempool.h"
#include <stdio.h>
#include <assert.h>

int main(void) {
    mempool_t *pool = mempool_create();
    assert(pool != NULL);

    void *a = mp_alloc(pool, 100);
    assert(a != NULL);
    assert(((unsigned long)a % sizeof(void *)) == 0);

    mp_free(pool, a);
    assert(mempool_allocated_count(pool) == 0);

    mempool_destroy(pool);

    printf("conan test_package: find_package(mempool) OK\n");
    return 0;
}
