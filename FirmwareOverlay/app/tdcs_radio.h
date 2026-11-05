#ifndef TDCS_RADIO_H
#define TDCS_RADIO_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define TDCS_RADIO_PACKET_BYTES 76u
#define TDCS_RADIO_QUEUE_DEPTH  2u

#define TDCS_RADIO_IRQ_FSK_SYNC          (1u << 1)
#define TDCS_RADIO_IRQ_FIFO_ALMOST_FULL  (1u << 12)
#define TDCS_RADIO_IRQ_RX_FINISHED       (1u << 13)

typedef uint16_t (*TDCS_RadioReadRegisterFn)(uint8_t reg);
typedef void (*TDCS_RadioWriteRegisterFn)(uint8_t reg, uint16_t value);
typedef void (*TDCS_RadioRxTurnOnFn)(void);

typedef struct {
    TDCS_RadioReadRegisterFn read_register;
    TDCS_RadioWriteRegisterFn write_register;
    TDCS_RadioRxTurnOnFn rx_turn_on;
} TDCS_RadioOps;

typedef struct {
    uint32_t received_frames;
    uint32_t dropped_frames;
    uint32_t incomplete_frames;
    uint32_t fifo_overflows;
} TDCS_RadioStats;

/*
 * Configure the BK4819 FFSK 1200/1800 receive modem.
 * The host firmware must tune the radio to the target 450 MHz channel first.
 */
bool TDCS_RadioInit(const TDCS_RadioOps *ops);

/* Call from the existing BK4819 CheckRadioInterrupts() path. */
void TDCS_RadioHandleInterrupt(uint16_t interrupt_bits);

/* Pop one completed 76-byte FIFO packet in main-loop context. */
bool TDCS_RadioPopFrame(uint8_t packet[TDCS_RADIO_PACKET_BYTES], bool *inverted);

void TDCS_RadioGetStats(TDCS_RadioStats *stats);

#endif
