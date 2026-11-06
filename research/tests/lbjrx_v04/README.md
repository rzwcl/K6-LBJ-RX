# LBJRX 0.4 original decoder host tests

These tests compile and execute the unmodified upstream `lbj_dec.c` from
`research/original_apps/LBJRX-RX04/`. The test source contains an independent
POCSAG BCH encoder solely to synthesize input vectors.

Run:

```sh
make test
```

Coverage:
- valid 1234000 address + 12345 BCD message;
- normal and inverted bitstream polarity;
- two-bit sync-word error tolerance;
- one-bit BCH correction;
- an uncorrectable two-bit codeword retained as five '?' placeholders.

This verifies the original software bit decoder only. It does not exercise RF,
BK4819 registers/FIFO, ADC, UI, timing, or any K6 hardware.
