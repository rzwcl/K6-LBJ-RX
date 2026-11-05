#include "lbj_pocsag.h"

#include <string.h>

#define POCSAG_GEN       0x769u
#define POCSAG_BATCH     16u
#define POCSAG_SYNC_ERR  2u
#define POCSAG_GAP       0x7Fu

static unsigned popcount32(uint32_t v)
{
    unsigned n = 0;
    while (v) {
        v &= v - 1u;
        ++n;
    }
    return n;
}

static uint32_t syndrome(uint32_t cw)
{
    uint32_t rem = cw >> 1;
    int i;
    for (i = 30; i >= 10; --i) {
        if (rem & (1u << (unsigned)i))
            rem ^= POCSAG_GEN << (unsigned)(i - 10);
    }
    return rem;
}

/* POCSAG BCH(31,21) plus odd parity. Returns 0=uncorrectable, 1=clean, 2=fixed. */
static unsigned correct_codeword(uint32_t *word)
{
    uint32_t cw = *word;
    uint32_t s = syndrome(cw);
    unsigned result = 1u;

    if (s != 0u) {
        uint32_t p = 1u;
        unsigned bit = 1u;
        while (p != s) {
            p <<= 1;
            if (p & 0x400u)
                p ^= POCSAG_GEN;
            if (++bit > 31u)
                return 0u;
        }
        cw ^= 1u << bit;
        result = 2u;
    }

    if ((popcount32(cw) & 1u) != 0u) {
        if (s != 0u)
            return 0u;
        cw ^= 1u;
        result = 2u;
    }

    *word = cw;
    return result;
}

static void append_char(LBJ_PocsagDecoder *d, char c)
{
    if (d->text_length < LBJ_POCSAG_TEXT_MAX)
        d->text[d->text_length++] = c;
}

static void close_page(LBJ_PocsagDecoder *d)
{
    LBJ_PocsagPage page;
    if (!d->page_open)
        return;

    memset(&page, 0, sizeof(page));
    page.ric = d->ric;
    page.received_ms = d->last_ms;
    page.function = d->function;
    page.numeric = d->numeric;
    page.length = d->text_length;
    page.corrected_words = d->corrected_words;
    page.bad_words = d->bad_words;
    if (page.length > LBJ_POCSAG_TEXT_MAX)
        page.length = LBJ_POCSAG_TEXT_MAX;
    memcpy(page.text, d->text, page.length);
    page.text[page.length] = '\0';

    d->page_open = 0;
    if (d->callback)
        d->callback(&page, d->callback_user);
}

static void reset_page_text(LBJ_PocsagDecoder *d)
{
    d->text_length = 0;
    d->numeric_nibble = 0;
    d->numeric_bits = 0;
    d->text_accumulator = 0;
    d->text_bits = 0;
    d->corrected_words = 0;
    d->bad_words = 0;
    memset(d->text, 0, sizeof(d->text));
}

static char numeric_character(unsigned n)
{
    static const char map[16] = {
        '0','1','2','3','4','5','6','7','8',
        '9','*','U',' ','-',')','('
    };
    return map[n & 0x0Fu];
}

static void append_payload(LBJ_PocsagDecoder *d, uint32_t cw)
{
    int bit;
    if (d->numeric) {
        for (bit = 30; bit >= 11; --bit) {
            d->numeric_nibble |= (uint8_t)(((cw >> (unsigned)bit) & 1u) << d->numeric_bits);
            if (++d->numeric_bits == 4u) {
                append_char(d, numeric_character(d->numeric_nibble));
                d->numeric_nibble = 0;
                d->numeric_bits = 0;
            }
        }
    } else {
        for (bit = 30; bit >= 11; --bit) {
            d->text_accumulator |= (uint8_t)(((cw >> (unsigned)bit) & 1u) << d->text_bits);
            if (++d->text_bits == 7u) {
                uint8_t c = d->text_accumulator;
                append_char(d, (c >= 32u && c <= 126u) ? (char)c : '?');
                d->text_accumulator = 0;
                d->text_bits = 0;
            }
        }
    }
}

static void process_codeword(LBJ_PocsagDecoder *d, uint32_t cw)
{
    unsigned ok = correct_codeword(&cw);
    if (!ok) {
        ++d->bad_words;
        return;
    }
    if (ok == 2u)
        ++d->corrected_words;

    if (cw == LBJ_POCSAG_IDLE) {
        close_page(d);
        return;
    }

    if ((cw & 0x80000000u) == 0u) {
        uint32_t ric = (((cw >> 13) & 0x3FFFFu) << 3) | ((d->word_index >> 1) & 7u);
        close_page(d);
        reset_page_text(d);
        d->ric = ric;
        d->function = (uint8_t)((cw >> 11) & 3u);
        d->numeric = (uint8_t)(d->function == 0u);
        d->page_open = 1;
        return;
    }

    if (d->page_open)
        append_payload(d, cw);
}

void LBJ_PocsagInit(LBJ_PocsagDecoder *d,
                    LBJ_PocsagPageCallback callback,
                    void *user)
{
    if (!d)
        return;
    memset(d, 0, sizeof(*d));
    d->callback = callback;
    d->callback_user = user;
}

void LBJ_PocsagReset(LBJ_PocsagDecoder *d)
{
    LBJ_PocsagPageCallback cb;
    void *user;
    if (!d)
        return;
    cb = d->callback;
    user = d->callback_user;
    memset(d, 0, sizeof(*d));
    d->callback = cb;
    d->callback_user = user;
}

void LBJ_PocsagFeedBit(LBJ_PocsagDecoder *d, uint8_t bit, uint32_t now_ms)
{
    unsigned e;
    if (!d)
        return;
    d->last_ms = now_ms;
    d->shift = (d->shift << 1) | (uint32_t)(bit & 1u);

    if (!d->synchronized) {
        e = popcount32(d->shift ^ LBJ_POCSAG_SYNC);
        if (e <= POCSAG_SYNC_ERR || e >= 32u - POCSAG_SYNC_ERR) {
            d->polarity = (e >= 16u) ? 0xFFFFFFFFu : 0u;
            d->synchronized = 1u;
            d->word_index = 0u;
            d->bit_count = 0u;
        }
        return;
    }

    d->codeword = (d->codeword << 1) | (uint32_t)(bit & 1u);
    if (++d->bit_count < 32u)
        return;
    d->bit_count = 0u;

    if (d->word_index == POCSAG_BATCH) {
        uint32_t sync = d->codeword ^ d->polarity;
        e = popcount32(sync ^ LBJ_POCSAG_SYNC);
        if (e <= POCSAG_SYNC_ERR || e >= 32u - POCSAG_SYNC_ERR) {
            d->polarity = (e >= 16u) ? 0xFFFFFFFFu : 0u;
            d->word_index = 0u;
        } else {
            close_page(d);
            d->synchronized = 0u;
            d->word_index = 0u;
            d->bit_count = 0u;
        }
        return;
    }

    process_codeword(d, d->codeword ^ d->polarity);
    ++d->word_index;
}

void LBJ_PocsagFeedBitsMSB(LBJ_PocsagDecoder *d,
                           const uint8_t *bytes, size_t length,
                           uint32_t now_ms)
{
    size_t i;
    unsigned bit;
    if (!d || !bytes)
        return;
    for (i = 0; i < length; ++i) {
        for (bit = 0; bit < 8u; ++bit)
            LBJ_PocsagFeedBit(d, (uint8_t)((bytes[i] >> (7u - bit)) & 1u), now_ms);
    }
}
