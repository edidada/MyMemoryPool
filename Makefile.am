## -*- Makefile.am -*-  (processed by automake into Makefile.in)

# A convenience static library that is NOT installed. The test links against it.
noinst_LIBRARIES = libmempool.a
libmempool_a_SOURCES = mempool.c mempool.h

# The test program. `make check` builds and runs it (TESTS).
check_PROGRAMS = test_mempool
test_mempool_SOURCES = test_mempool.c
test_mempool_LDADD = libmempool.a

# Register the test so `make check` executes it.
TESTS = test_mempool
