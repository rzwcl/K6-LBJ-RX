#ifndef STUB_UI_HELPER_H
#define STUB_UI_HELPER_H
#include <stdint.h>
void UI_DisplayClear(void);
void UI_PrintStringSmall(const char *s, uint8_t start, uint8_t end, uint8_t line);
void UI_DisplayFrequency(const char *s, uint8_t x, uint8_t y, bool center);
void GUI_DisplaySmallest(const char *s, uint8_t x, uint8_t y, bool statusbar, bool fill);
#endif
