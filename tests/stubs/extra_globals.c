#include <stdbool.h>
#include <stdint.h>
#include "ui/ui.h"
#include "driver/st7565.h"
bool gUpdateDisplay;
GUI_DisplayType_t gRequestDisplayScreen;
uint8_t gStatusLine[LCD_WIDTH];
uint8_t gFrameBuffer[FRAME_LINES][LCD_WIDTH];

volatile uint16_t gFlashLightBlinkCounter;
