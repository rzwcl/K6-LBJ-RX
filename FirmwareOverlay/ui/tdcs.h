#ifndef TDCS_UI_H
#define TDCS_UI_H

#include <stdint.h>
#include "driver/keyboard.h"

void UI_DisplayTDCS(void);
void TDCS_ProcessKeys(KEY_Code_t key, bool pressed, bool held);

#endif
