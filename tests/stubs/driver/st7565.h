#ifndef STUB_ST7565_H
#define STUB_ST7565_H
#include <stdint.h>
#define LCD_WIDTH 128
#define FRAME_LINES 7
extern uint8_t gStatusLine[LCD_WIDTH];
extern uint8_t gFrameBuffer[FRAME_LINES][LCD_WIDTH];
void ST7565_BlitStatusLine(void);
void ST7565_BlitFullScreen(void);
#endif
