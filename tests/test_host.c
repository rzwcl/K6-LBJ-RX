#include "lbj_pocsag.h"
#include "lbj_train_type.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static unsigned callbacks;
static LBJ_PocsagPage last_page;

static unsigned popcount32(uint32_t v)
{
    unsigned n = 0;
    while (v) { v &= v - 1u; ++n; }
    return n;
}

static uint32_t syndrome(uint32_t cw)
{
    uint32_t r = cw >> 1;
    int i;
    for (i = 30; i >= 10; --i)
        if (r & (1u << (unsigned)i)) r ^= 0x769u << (unsigned)(i - 10);
    return r;
}

static uint32_t encode_data(uint32_t data)
{
    uint32_t cw = data << 11;
    cw |= syndrome(cw) << 1;
    if ((popcount32(cw) & 1u) != 0u) cw ^= 1u;
    return cw;
}

static void on_page(const LBJ_PocsagPage *p, void *user)
{
    (void)user;
    ++callbacks;
    last_page = *p;
}

static void feed_word(LBJ_PocsagDecoder *d, uint32_t word, uint32_t *t)
{
    int i;
    for (i = 31; i >= 0; --i)
        LBJ_PocsagFeedBit(d, (uint8_t)((word >> (unsigned)i) & 1u), (*t)++);
}

static uint32_t text_message_word(const char *text)
{
    uint32_t data = 1u << 20;
    unsigned i, bit;
    /* POCSAG text characters enter the 20-bit payload least-significant bit first. */
    for (i = 0; text[i] && i < 2u; ++i) {
        for (bit = 0; bit < 7u; ++bit) {
            unsigned stream_bit = i * 7u + bit;
            if ((text[i] >> bit) & 1u)
                data |= 1u << (19u - stream_bit);
        }
    }
    return encode_data(data);
}

static uint32_t numeric_message_word(const char *text, unsigned offset)
{
    uint32_t data = 1u << 20;
    unsigned i, bit;
    for (i = 0; i < 5u && text[offset + i]; ++i) {
        unsigned value;
        char c = text[offset + i];
        if (c >= '0' && c <= '9') value = (unsigned)(c - '0');
        else if (c == ' ') value = 12u;
        else if (c == '-') value = 13u;
        else value = 0u;
        for (bit = 0; bit < 4u; ++bit) {
            unsigned stream_bit = i * 4u + bit;
            if ((value >> bit) & 1u)
                data |= 1u << (19u - stream_bit);
        }
    }
    return encode_data(data);
}

static void feed_word_inverted(LBJ_PocsagDecoder *d, uint32_t word, uint32_t *t)
{
    int i;
    for (i = 31; i >= 0; --i)
        LBJ_PocsagFeedBit(d, (uint8_t)(1u ^ ((word >> (unsigned)i) & 1u)), (*t)++);
}

static void test_pocsag(void)
{
    LBJ_PocsagDecoder d;
    uint32_t t = 0;
    uint32_t address_data = (((1234000u >> 3) & 0x3FFFFu) << 2) | 1u;
    uint32_t address = encode_data(address_data);
    uint32_t msg = text_message_word("OK") ^ (1u << 7); /* one bit error */
    LBJ_PocsagInit(&d, on_page, 0);
    callbacks = 0;
    feed_word(&d, LBJ_POCSAG_SYNC, &t);
    feed_word(&d, address, &t);
    feed_word(&d, msg, &t);
    feed_word(&d, LBJ_POCSAG_IDLE, &t);
    assert(callbacks == 1u);
    assert(last_page.ric == 1234000u);
    assert(last_page.function == 1u);
    assert(last_page.numeric == 0u);
    assert(strcmp(last_page.text, "OK") == 0);
    assert(last_page.corrected_words == 1u);

    /* Numeric page: two words contain five 4-bit symbols each. */
    LBJ_PocsagReset(&d);
    t = 1000u;
    address_data = ((1234000u >> 3) & 0x3FFFFu) << 2; /* function 0 */
    address = encode_data(address_data);
    callbacks = 0;
    feed_word(&d, LBJ_POCSAG_SYNC, &t);
    feed_word(&d, address, &t);
    feed_word(&d, numeric_message_word("12340", 0u), &t);
    feed_word(&d, numeric_message_word("00   ", 0u), &t);
    feed_word(&d, LBJ_POCSAG_IDLE, &t);
    assert(callbacks == 1u);
    assert(last_page.ric == 1234000u);
    assert(last_page.function == 0u);
    assert(last_page.numeric == 1u);
    assert(strcmp(last_page.text, "1234000   ") == 0);

    /* Reversed discriminator polarity must produce the same page. */
    LBJ_PocsagReset(&d);
    t = 2000u;
    callbacks = 0;
    address_data = (((1234000u >> 3) & 0x3FFFFu) << 2) | 1u;
    address = encode_data(address_data);
    feed_word_inverted(&d, LBJ_POCSAG_SYNC, &t);
    feed_word_inverted(&d, address, &t);
    feed_word_inverted(&d, msg ^ (1u << 7), &t); /* restore the clean text word */
    feed_word_inverted(&d, LBJ_POCSAG_IDLE, &t);
    assert(callbacks == 1u);
    assert(strcmp(last_page.text, "OK") == 0);
}

static void expect_type(const char *no, const char *needle)
{
    char out[64];
    LBJ_IdentifyTrainType(no, out, sizeof(out));
    if (!strstr(out, needle)) {
        fprintf(stderr, "train type mismatch: %s -> [%s], expected substring [%s]\n", no, out, needle);
        assert(0);
    }
}

static void test_train_types(void)
{
    expect_type("K1206", "\xBF\xEC\xCB\xD9\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5");
    expect_type("X21", "\xD0\xC2\xB8\xDF\xCB\xD9\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5");
    expect_type("X55001", "\xD0\xC2\xB8\xDF\xCB\xD9\xCA\xD4\xD4\xCB\xD0\xD0\xC1\xD0\xB3\xB5");
    expect_type("DJ1998", "\xB6\xAF\xB3\xB5\xD7\xE9\xBC\xEC\xB2\xE2\xC1\xD0\xB3\xB5");
    expect_type("DJ2001", "\xB6\xAF\xB3\xB5\xD7\xE9\xC8\xB7\xC8\xCF\xC1\xD0\xB3\xB5");
    expect_type("DH1", "\xB6\xAF\xB3\xB5\xD7\xE9\xBF\xEC\xD4\xCB\xC1\xD0\xB3\xB5");
    expect_type("B201", "\xBF\xEC\xCB\xD9\xB6\xE0\xCA\xBD\xC1\xAA\xD4\xCB\xB0\xE0\xC1\xD0");
    expect_type("00123", "\xD3\xD0\xB6\xAF\xC1\xA6\xBB\xD8\xCB\xCD\xB6\xAF\xB3\xB5\xD7\xE9");
    expect_type("00501", "\xCE\xDE\xB6\xAF\xC1\xA6\xBB\xD8\xCB\xCD\xB6\xAF\xB3\xB5\xD7\xE9");
    expect_type("57001", "\xC2\xB7\xD3\xC3\xC1\xD0\xB3\xB5");
    expect_type("FG21", "\xD5\xDB\xB7\xB5\xC1\xD0\xB3\xB5");
    expect_type("0K1206", "\xBF\xEC\xCB\xD9\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5(\xBB\xD8\xCB\xCD)");
    expect_type("FK1206", "\xD5\xDB\xB7\xB5\xC1\xD0\xB3\xB5");
}

int main(void)
{
    test_pocsag();
    test_train_types();
    puts("host tests: PASS");
    return 0;
}
