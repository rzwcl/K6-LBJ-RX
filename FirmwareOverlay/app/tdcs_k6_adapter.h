#ifndef TDCS_K6_ADAPTER_H
#define TDCS_K6_ADAPTER_H

#include <stdint.h>

void TDCS_K6_Start(void);
void TDCS_K6_HandleInterrupts(uint16_t interrupt_bits);
void TDCS_K6_Process(void);

#endif
