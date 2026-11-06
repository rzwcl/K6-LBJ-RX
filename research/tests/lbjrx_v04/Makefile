CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror

SOURCE_DIR = ../../original_apps/LBJRX-RX04
TEST_BIN = test_lbj_dec_host

.PHONY: all test clean

all: test

$(TEST_BIN): test_lbj_dec_host.c $(SOURCE_DIR)/lbj_dec.c $(SOURCE_DIR)/lbj_dec.h
	$(CC) $(CFLAGS) -I$(SOURCE_DIR) -o $@ test_lbj_dec_host.c $(SOURCE_DIR)/lbj_dec.c

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(TEST_BIN)
