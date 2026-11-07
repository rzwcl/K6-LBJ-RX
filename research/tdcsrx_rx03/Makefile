CC ?= cc
CFLAGS ?= -std=c99 -O2 -Wall -Wextra -Werror

CORE = tdcsrx_recovered_protocol.c tdcsrx_recovered_stream.c tdcsrx_recovered_receiver.c tdcsrx_recovered_dsp.c
TEST_CORE = test_tdcsrx_recovered.c $(CORE)
TEST_RECEIVER = test_tdcsrx_recovered_receiver.c $(CORE)
TEST_DSP = test_tdcsrx_recovered_dsp.c $(CORE)
TEST_REAL_WAV = test_tdcsrx_real_wav.c $(CORE)

.PHONY: all test test-protocol test-dsp probe-real-wav clean

all: test

test: test-protocol

test_tdcsrx_recovered: $(TEST_CORE) tdcsrx_recovered_protocol.h tdcsrx_recovered_stream.h tdcsrx_recovered_receiver.h tdcsrx_recovered_dsp.h
	$(CC) $(CFLAGS) -o $@ $(TEST_CORE)

test_tdcsrx_recovered_receiver: $(TEST_RECEIVER) tdcsrx_recovered_protocol.h tdcsrx_recovered_stream.h tdcsrx_recovered_receiver.h tdcsrx_recovered_dsp.h
	$(CC) $(CFLAGS) -o $@ $(TEST_RECEIVER)

test_tdcsrx_recovered_dsp: $(TEST_DSP) tdcsrx_recovered_protocol.h tdcsrx_recovered_stream.h tdcsrx_recovered_receiver.h tdcsrx_recovered_dsp.h
	$(CC) $(CFLAGS) -o $@ $(TEST_DSP)

test_tdcsrx_real_wav: $(TEST_REAL_WAV) tdcsrx_recovered_protocol.h tdcsrx_recovered_stream.h tdcsrx_recovered_receiver.h tdcsrx_recovered_dsp.h
	$(CC) $(CFLAGS) -o $@ $(TEST_REAL_WAV) -lm

probe-real-wav: test_tdcsrx_real_wav
	@test -n "$(SAMPLE_BIN)" || (echo "SAMPLE_BIN must name a prepared ADC sample file" >&2; exit 2)
	./test_tdcsrx_real_wav "$(SAMPLE_BIN)"

# Validated protocol/frame layer: BCH, sync, collectors, frame and CRC.
test-protocol: test_tdcsrx_recovered test_tdcsrx_recovered_receiver
	./test_tdcsrx_recovered
	./test_tdcsrx_recovered_receiver

# Incomplete sample-to-bit reconstruction. This diagnostic currently fails
# its end-to-end synthetic audio-to-frame assertion; keep it separate.
test-dsp: test_tdcsrx_recovered_dsp
	./test_tdcsrx_recovered_dsp

clean:
	rm -f test_tdcsrx_recovered test_tdcsrx_recovered_receiver test_tdcsrx_recovered_dsp test_tdcsrx_real_wav
