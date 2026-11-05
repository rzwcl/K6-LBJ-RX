#ifndef TDCS_APP_H
#define TDCS_APP_H

#include "tdcs_protocol.h"
#include "tdcs_radio.h"

#include <stdbool.h>
#include <stdint.h>

#define TDCS_HISTORY_CAPACITY 8u

typedef struct {
    uint8_t valid;
    uint8_t corrected_bits;
    uint8_t inverted;
    uint8_t bit_order_lsb;
    uint32_t received_ms;
    uint8_t payload[TDCS_PAYLOAD_SIZE];
    TDCS_PayloadFields fields;
} TDCS_TrainRecord;

typedef struct {
    uint32_t accepted_frames;
    uint32_t rejected_frames;
    uint32_t rejected_bch;
    uint32_t rejected_header;
    uint32_t rejected_padding;
    uint32_t rejected_crc;
    TDCS_RadioStats radio;
} TDCS_AppStats;

bool TDCS_AppInit(const TDCS_RadioOps *radio_ops);
void TDCS_AppProcess(uint32_t now_ms);

uint8_t TDCS_AppGetHistoryCount(void);
uint8_t TDCS_AppGetSelectedIndex(void);
const TDCS_TrainRecord *TDCS_AppGetRecord(uint8_t index);
const TDCS_TrainRecord *TDCS_AppGetSelected(void);

void TDCS_AppMoveSelection(int direction);
void TDCS_AppToggleDetail(void);
uint8_t TDCS_AppGetDetailPage(void);
void TDCS_AppGetStats(TDCS_AppStats *stats);

#endif
