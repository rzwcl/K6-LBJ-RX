CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Werror -pedantic -O2

.PHONY: all test clean
all: test

test: test_tdcsrx_recovered
	./test_tdcsrx_recovered

test_tdcsrx_recovered: tdcsrx_recovered_protocol.c tdcsrx_recovered_protocol.h test_tdcsrx_recovered.c
	$(CC) $(CFLAGS) tdcsrx_recovered_protocol.c test_tdcsrx_recovered.c -o $@

clean:
	rm -f test_tdcsrx_recovered
