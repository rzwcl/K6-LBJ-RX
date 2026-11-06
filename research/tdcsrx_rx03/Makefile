CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror

CORE = tdcsrx_recovered_protocol.c tdcsrx_recovered_stream.c tdcsrx_recovered_receiver.c tdcsrx_recovered_dsp.c
TEST_CORE = test_tdcsrx_recovered.c $(CORE)
TEST_RECEIVER = test_tdcsrx_recovered_receiver.c $(CORE)
TEST_DSP = test_tdcsrx_recovered_dsp.c $(CORE)

.PHONY: all test clean

all: test

test_tdcsrx_recovered: $(TEST_CORE) tdcsrx_recovered_protocol.h tdcsrx_recovered_stream.h tdcsrx_recovered_receiver.h tdcsrx_recovered_dsp.h
	$(CC) $(CFLAGS) -o $@ $(TEST_CORE)

test_tdcsrx_recovered_receiver: $(TEST_RECEIVER) tdcsrx_recovered_protocol.h tdcsrx_recovered_stream.h tdcsrx_recovered_receiver.h tdcsrx_recovered_dsp.h
	$(CC) $(CFLAGS) -o $@ $(TEST_RECEIVER)

test_tdcsrx_recovered_dsp: $(TEST_DSP) tdcsrx_recovered_protocol.h tdcsrx_recovered_stream.h tdcsrx_recovered_receiver.h tdcsrx_recovered_dsp.h
	$(CC) $(CFLAGS) -o $@ $(TEST_DSP)

test: test_tdcsrx_recovered test_tdcsrx_recovered_receiver test_tdcsrx_recovered_dsp
	./test_tdcsrx_recovered
	./test_tdcsrx_recovered_receiver
	./test_tdcsrx_recovered_dsp

clean:
	rm -f test_tdcsrx_recovered test_tdcsrx_recovered_receiver test_tdcsrx_recovered_dsp
