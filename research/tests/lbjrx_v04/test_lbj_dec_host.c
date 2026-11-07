#include "lbj_dec.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define POC_SYNC 0x7CD215D8u
#define POC_IDLE 0x7A89C197u
#define POC_BCH_POLY 0x769u

typedef struct {
    unsigned int calls;
    uint32_t addr;
    uint8_t func;
    uint8_t err;
    uint8_t cor;
    uint16_t len;
    char bcd[LBJ_CWMAX * 5u + 1u];
} capture_t;

static capture_t g_capture;

void lbj_emit_msg(const lbj_rx_t *rx, const char *bcd, uint16_t len)
{
    ++g_capture.calls;
    g_capture.addr = rx->addr;
    g_capture.func = rx->func;
    g_capture.err = rx->err;
    g_capture.cor = rx->cor;
    g_capture.len = len;

    if (len > sizeof(g_capture.bcd) - 1u)
        len = (uint16_t)(sizeof(g_capture.bcd) - 1u);
    memcpy(g_capture.bcd, bcd, len);
    g_capture.bcd[len] = '\0';
}

static unsigned int popcount32(uint32_t value)
{
    unsigned int count = 0u;
    while (value != 0u) {
        value &= value - 1u;
        ++count;
    }
    return count;
}

/* Independent POCSAG BCH(31,21) encoder used only to make test vectors. */
static uint32_t encode_codeword(uint8_t id, uint32_t data20)
{
    const uint32_t data21 =
        (((uint32_t)(id & 1u)) << 20) | (data20 & 0x000fffffu);
    uint32_t remainder = data21 << 10;
    int bit;
    uint32_t code31;
    uint32_t codeword;
    uint32_t parity;

    for (bit = 30; bit >= 10; --bit) {
        if ((remainder & (1u << (unsigned int)bit)) != 0u)
            remainder ^= POC_BCH_POLY << (unsigned int)(bit - 10);
    }

    code31 = (data21 << 10) | (remainder & 0x03ffu);
    parity = popcount32(code31) & 1u; /* even parity across all 32 bits */
    codeword = (code31 << 1) | parity;
    assert((popcount32(codeword) & 1u) == 0u);
    return codeword;
}

static uint8_t reverse4(uint8_t value)
{
    return (uint8_t)(((value & 0x1u) << 3) |
                     ((value & 0x2u) << 1) |
                     ((value & 0x4u) >> 1) |
                     ((value & 0x8u) >> 3));
}

static uint32_t encode_bcd5(const char digits[5])
{
    uint32_t value = 0u;
    unsigned int i;
    for (i = 0u; i < 5u; ++i) {
        assert(digits[i] >= '0' && digits[i] <= '9');
        value = (value << 4) | reverse4((uint8_t)(digits[i] - '0'));
    }
    return value;
}

static void feed_word(lbj_rx_t *rx, uint32_t word, bool inverted)
{
    unsigned int bit;
    for (bit = 0u; bit < 32u; ++bit) {
        uint8_t value = (uint8_t)((word >> (31u - bit)) & 1u);
        if (inverted)
            value ^= 1u;
        lbj_rx_bit(rx, value);
    }
}

static void run_one_report(bool inverted, unsigned int sync_errors,
                           unsigned int message_error_mask)
{
    lbj_rx_t rx;
    uint32_t ric = 1234000u;
    uint32_t data20;
    uint32_t address_word;
    uint32_t message_word;
    unsigned int i;

    memset(&rx, 0, sizeof(rx));
    memset(&g_capture, 0, sizeof(g_capture));

    /* Address data20 = ((RIC >> 3) << 2) | function. */
    data20 = ((ric >> 3) << 2) | 1u;
    address_word = encode_codeword(0u, data20);
    message_word = encode_codeword(1u, encode_bcd5("12345"));

    if (message_error_mask != 0u)
        message_word ^= message_error_mask;

    /* POCSAG frame sync, optionally with up to two physical bit errors. */
    for (i = 0u; i < 32u; ++i) {
        uint8_t value = (uint8_t)((POC_SYNC >> (31u - i)) & 1u);
        if (i < 32u && (sync_errors & (1u << (i % 32u))) != 0u)
            value ^= 1u;
        if (inverted)
            value ^= 1u;
        lbj_rx_bit(&rx, value);
    }

    feed_word(&rx, address_word, inverted);
    feed_word(&rx, message_word, inverted);
    for (i = 2u; i < 16u; ++i)
        feed_word(&rx, POC_IDLE, inverted);
}

static void test_residual_pocsag_preamble_after_hardware_sync(void)
{
    lbj_rx_t rx;
    uint32_t ric = 1234000u;
    uint32_t address_word;
    uint32_t message_word;
    uint32_t data20;
    unsigned int i;
    unsigned int bit;
    unsigned int polarity;

    /*
     * Standard POCSAG preamble is 576 alternating bits. Simulate a two-byte
     * hardware sync consuming the first 16 bits, leaving 560 bits before the
     * original 32-bit frame sync reaches the software decoder.
     */
    for (polarity = 0u; polarity < 2u; ++polarity) {
        memset(&rx, 0, sizeof(rx));
        memset(&g_capture, 0, sizeof(g_capture));

        for (bit = 0u; bit < 560u; ++bit) {
            uint8_t value = (uint8_t)((bit & 1u) == 0u);
            if (polarity != 0u)
                value ^= 1u;
            lbj_rx_bit(&rx, value);
        }

        data20 = ((ric >> 3) << 2) | 1u;
        address_word = encode_codeword(0u, data20);
        message_word = encode_codeword(1u, encode_bcd5("12345"));

        for (i = 0u; i < 32u; ++i) {
            uint8_t value = (uint8_t)((POC_SYNC >> (31u - i)) & 1u);
            if (polarity != 0u)
                value ^= 1u;
            lbj_rx_bit(&rx, value);
        }

        feed_word(&rx, address_word, polarity != 0u);
        feed_word(&rx, message_word, polarity != 0u);
        for (i = 2u; i < 16u; ++i)
            feed_word(&rx, POC_IDLE, polarity != 0u);

        assert(g_capture.calls == 1u);
        assert(g_capture.addr == ric);
        assert(g_capture.len == 5u);
        assert(strcmp(g_capture.bcd, "12345") == 0);
    }
}

static void test_normal_polarity(void)
{
    run_one_report(false, 0u, 0u);
    assert(g_capture.calls == 1u);
    assert(g_capture.addr == 1234000u);
    assert(g_capture.func == 1u);
    assert(g_capture.len == 5u);
    assert(strcmp(g_capture.bcd, "12345") == 0);
    assert(g_capture.err == 0u);
    assert(g_capture.cor == 0u);
}

static void test_inverted_polarity(void)
{
    run_one_report(true, 0u, 0u);
    assert(g_capture.calls == 1u);
    assert(g_capture.addr == 1234000u);
    assert(strcmp(g_capture.bcd, "12345") == 0);
    assert(g_capture.err == 0u);
    assert(g_capture.cor == 0u);
}

static void test_two_bit_sync_tolerance(void)
{
    /* Flip sync bits 1 and 19; lbj_rx_bit() accepts Hamming distance <= 2. */
    run_one_report(false, (1u << 1) | (1u << 19), 0u);
    assert(g_capture.calls == 1u);
    assert(g_capture.addr == 1234000u);
    assert(strcmp(g_capture.bcd, "12345") == 0);
}

static void test_single_bit_bch_repair(void)
{
    run_one_report(false, 0u, 1u << 5);
    assert(g_capture.calls == 1u);
    assert(strcmp(g_capture.bcd, "12345") == 0);
    assert(g_capture.err == 0u);
    assert(g_capture.cor == 1u);
}

static void test_uncorrectable_word_erasure(void)
{
    /* This two-bit pattern is uncorrectable by the original one-bit decoder. */
    run_one_report(false, 0u, (1u << 5) | (1u << 18));
    assert(g_capture.calls == 1u);
    assert(g_capture.len == 5u);
    assert(strcmp(g_capture.bcd, "?????") == 0);
    assert(g_capture.err == 1u);
    assert(g_capture.cor == 0u);
}

int main(void)
{
    test_residual_pocsag_preamble_after_hardware_sync();
    test_normal_polarity();
    test_inverted_polarity();
    test_two_bit_sync_tolerance();
    test_single_bit_bch_repair();
    test_uncorrectable_word_erasure();

    puts("LBJRX 0.4 original POCSAG decoder host tests: PASS");
    puts("- normal and inverted stream polarity");
    puts("- two-bit frame-sync tolerance");
    puts("- single-bit BCH correction");
    puts("- uncorrectable codeword preserved as five '?' markers");
    puts("- 560 residual alternating preamble bits after 16-bit hardware sync");
    return 0;
}
