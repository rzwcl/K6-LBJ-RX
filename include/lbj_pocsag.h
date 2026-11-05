#ifndef LBJ_POCSAG_H
#define LBJ_POCSAG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LBJ_POCSAG_TEXT_MAX 80u
#define LBJ_POCSAG_SYNC 0x7CD215D8u
#define LBJ_POCSAG_IDLE 0x7A89C197u

typedef struct {
    uint32_t ric;
    uint32_t received_ms;
    uint8_t function;
    uint8_t numeric;
    uint8_t length;
    char text[LBJ_POCSAG_TEXT_MAX + 1u];
    uint16_t corrected_words;
    uint16_t bad_words;
} LBJ_PocsagPage;

typedef void (*LBJ_PocsagPageCallback)(const LBJ_PocsagPage *page, void *user);

typedef struct {
    uint32_t shift;
    uint32_t polarity;
    uint32_t codeword;
    uint32_t last_ms;
    uint8_t bit_count;
    uint8_t word_index;
    uint8_t synchronized;
    uint8_t page_open;
    uint8_t numeric;
    uint8_t function;
    uint8_t text_length;
    uint8_t numeric_nibble;
    uint8_t numeric_bits;
    uint8_t text_accumulator;
    uint8_t text_bits;
    uint16_t corrected_words;
    uint16_t bad_words;
    uint32_t ric;
    char text[LBJ_POCSAG_TEXT_MAX + 1u];
    LBJ_PocsagPageCallback callback;
    void *callback_user;
} LBJ_PocsagDecoder;

void LBJ_PocsagInit(LBJ_PocsagDecoder *decoder,
                    LBJ_PocsagPageCallback callback,
                    void *user);
void LBJ_PocsagReset(LBJ_PocsagDecoder *decoder);
void LBJ_PocsagFeedBit(LBJ_PocsagDecoder *decoder, uint8_t bit, uint32_t now_ms);
void LBJ_PocsagFeedBitsMSB(LBJ_PocsagDecoder *decoder,
                           const uint8_t *bytes, size_t length,
                           uint32_t now_ms);

#endif
