#ifndef APP_LBJ_RADIO_H
#define APP_LBJ_RADIO_H

#include <stdint.h>

/* Configure the BK4819 receive-only FSK FIFO path for a POCSAG burst. */
void LBJ_RFStart(void);
/* Call from CheckRadioInterrupts() with the REG_02 interrupt status word. */
void LBJ_RFHandleInterrupts(uint16_t interrupt_bits);

#endif
