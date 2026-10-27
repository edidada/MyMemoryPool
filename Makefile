CC      ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c11 -g

TARGET  := test_mempool

.PHONY: all run clean

all: $(TARGET)

$(TARGET): test_mempool.c mempool.c mempool.h
	$(CC) $(CFLAGS) -o $@ test_mempool.c mempool.c

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET) test_asan
