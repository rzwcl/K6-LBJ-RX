#ifndef LBJ_APP_H
#define LBJ_APP_H

#include <stdbool.h>
#include <stdint.h>
#include "driver/keyboard.h"
#include "lbj_core.h"

void LBJ_AppInit(void);
void LBJ_AppProcess(void);
bool LBJ_AppBootSplashActive(void);
void LBJ_ProcessKeys(KEY_Code_t key, bool pressed, bool held);
/* RF adapter boundary: each new BK4819 sync starts a fresh POCSAG stream. */
void LBJ_AppBeginRFFrame(bool inverted, uint32_t now_ms);
void LBJ_AppQueueRFWord(uint16_t word, bool inverted, uint32_t now_ms);
/* Low-level queue entry retained for unit tests and future bit-slicer adapters. */
void LBJ_AppQueueBit(uint8_t bit, uint32_t now_ms);
const LBJ_Engine *LBJ_AppGetEngine(void);

#endif
