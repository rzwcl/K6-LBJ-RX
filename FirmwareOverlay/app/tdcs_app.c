#include "tdcs_app.h"

#include <string.h>

static TDCS_TrainRecord gHistory[TDCS_HISTORY_CAPACITY];
static uint8_t gHistoryCount;
static uint8_t gSelectedIndex;
static uint8_t gDetailPage;
static TDCS_AppStats gStats;

static bool same_train(
    const TDCS_TrainRecord *record,
    const TDCS_PayloadFields *fields)
{
    return record->valid != 0u &&
           record->fields.train_number == fields->train_number &&
           record->fields.loco_model == fields->loco_model &&
           record->fields.loco_model_ext == fields->loco_model_ext &&
           record->fields.loco_number == fields->loco_number;
}

static void move_record_to_front(uint8_t index)
{
    TDCS_TrainRecord record;
    uint8_t i;

    if (index == 0u || index >= gHistoryCount) {
        return;
    }

    record = gHistory[index];
    for (i = index; i > 0u; --i) {
        gHistory[i] = gHistory[i - 1u];
    }
    gHistory[0] = record;
}

static void add_record(
    const TDCS_Frame *frame,
    const TDCS_PayloadFields *fields,
    bool inverted,
    bool bit_order_lsb,
    uint32_t now_ms)
{
    uint8_t i;
    uint8_t target = TDCS_HISTORY_CAPACITY;

    for (i = 0u; i < gHistoryCount; ++i) {
        if (same_train(&gHistory[i], fields)) {
            target = i;
            break;
        }
    }

    if (target < TDCS_HISTORY_CAPACITY) {
        move_record_to_front(target);
    } else {
        uint8_t last = gHistoryCount;

        if (last >= TDCS_HISTORY_CAPACITY) {
            last = (uint8_t)(TDCS_HISTORY_CAPACITY - 1u);
        } else {
            ++gHistoryCount;
        }

        for (i = last; i > 0u; --i) {
            gHistory[i] = gHistory[i - 1u];
        }
    }

    memset(&gHistory[0], 0, sizeof(gHistory[0]));
    gHistory[0].valid = 1u;
    gHistory[0].corrected_bits = frame->corrected_bits;
    gHistory[0].inverted = inverted ? 1u : 0u;
    gHistory[0].bit_order_lsb = bit_order_lsb ? 1u : 0u;
    gHistory[0].received_ms = now_ms;
    memcpy(gHistory[0].payload, frame->payload, TDCS_PAYLOAD_SIZE);
    gHistory[0].fields = *fields;

    gSelectedIndex = 0u;
}

bool TDCS_AppInit(const TDCS_RadioOps *radio_ops)
{
    memset(gHistory, 0, sizeof(gHistory));
    memset(&gStats, 0, sizeof(gStats));
    gHistoryCount = 0u;
    gSelectedIndex = 0u;
    gDetailPage = 0u;

    return TDCS_RadioInit(radio_ops);
}

void TDCS_AppProcess(uint32_t now_ms)
{
    uint8_t packet[TDCS_RADIO_PACKET_BYTES];
    bool inverted;

    while (TDCS_RadioPopFrame(packet, &inverted)) {
        TDCS_Frame frame;
        TDCS_PayloadFields fields;
        TDCS_DecodeResult msb_result;
        TDCS_DecodeResult lsb_result;
        bool lsb_first = false;
        bool decoded = false;

        msb_result = TDCS_DecodePackedFrame(
            packet, sizeof(packet), 0u, false, &frame);

        if (msb_result == TDCS_DECODE_OK) {
            decoded = true;
        } else {
            lsb_result = TDCS_DecodePackedFrame(
                packet, sizeof(packet), 0u, true, &frame);
            if (lsb_result == TDCS_DECODE_OK) {
                decoded = true;
                lsb_first = true;
            } else {
                TDCS_DecodeResult reason = msb_result;

                /* Prefer a frame-integrity failure over an earlier bit-order
                   BCH failure when recording a rejected packet. */
                if (lsb_result == TDCS_DECODE_BAD_CRC ||
                    lsb_result == TDCS_DECODE_BAD_HEADER ||
                    lsb_result == TDCS_DECODE_BAD_PADDING) {
                    reason = lsb_result;
                }

                ++gStats.rejected_frames;
                if (reason == TDCS_DECODE_BCH_UNCORRECTABLE) {
                    ++gStats.rejected_bch;
                } else if (reason == TDCS_DECODE_BAD_HEADER) {
                    ++gStats.rejected_header;
                } else if (reason == TDCS_DECODE_BAD_PADDING) {
                    ++gStats.rejected_padding;
                } else if (reason == TDCS_DECODE_BAD_CRC) {
                    ++gStats.rejected_crc;
                }
            }
        }

        if (!decoded) {
            continue;
        }

        if (!TDCS_ParsePayload(frame.payload, &fields)) {
            ++gStats.rejected_frames;
            continue;
        }

        add_record(&frame, &fields, inverted, lsb_first, now_ms);
        ++gStats.accepted_frames;
    }

    TDCS_RadioGetStats(&gStats.radio);
}

uint8_t TDCS_AppGetHistoryCount(void)
{
    return gHistoryCount;
}

uint8_t TDCS_AppGetSelectedIndex(void)
{
    return gSelectedIndex;
}

const TDCS_TrainRecord *TDCS_AppGetRecord(uint8_t index)
{
    if (index >= gHistoryCount) {
        return NULL;
    }
    return &gHistory[index];
}

const TDCS_TrainRecord *TDCS_AppGetSelected(void)
{
    return TDCS_AppGetRecord(gSelectedIndex);
}

void TDCS_AppMoveSelection(int direction)
{
    if (gHistoryCount == 0u || direction == 0) {
        return;
    }

    if (direction < 0) {
        if (gSelectedIndex == 0u) {
            gSelectedIndex = (uint8_t)(gHistoryCount - 1u);
        } else {
            --gSelectedIndex;
        }
    } else {
        ++gSelectedIndex;
        if (gSelectedIndex >= gHistoryCount) {
            gSelectedIndex = 0u;
        }
    }
}

void TDCS_AppToggleDetail(void)
{
    gDetailPage ^= 1u;
}

uint8_t TDCS_AppGetDetailPage(void)
{
    return gDetailPage;
}

void TDCS_AppGetStats(TDCS_AppStats *stats)
{
    if (stats != NULL) {
        *stats = gStats;
        TDCS_RadioGetStats(&stats->radio);
    }
}
