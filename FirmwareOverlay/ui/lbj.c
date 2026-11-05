#include "lbj.h"
#include "app/lbj_app.h"
#include "app/lbj_core.h"

#include <stdio.h>
#include <string.h>

#include "driver/st7565.h"
#include "ui/helper.h"

static void print_line(const char *s, uint8_t line)
{
    if (s && s[0])
        UI_PrintStringSmall(s, 0u, 127u, line);
}

/* UI_PrintStringSmall uses 13 pixels per GB2312 glyph and 7 per ASCII glyph. */
static uint16_t text_pixel_width(const char *s)
{
    uint16_t width = 0u;
    const unsigned char *p = (const unsigned char *)s;

    while (p && *p) {
        if (*p >= 0x80u) {
            width = (uint16_t)(width + 13u);
            p += (p[1] != 0u) ? 2u : 1u;
        } else {
            width = (uint16_t)(width + 7u);
            ++p;
        }
    }
    return width;
}

static void format_km_display(char *dst, size_t cap, const char *km)
{
    size_t n = 0u;

    if (!dst || cap < 3u)
        return;

    dst[0] = 'K';
    if (km) {
        while (km[n] && km[n] != '.' && n + 2u < cap) {
            dst[n + 1u] = km[n];
            ++n;
        }
    }
    if (n == 0u) {
        dst[1] = '-';
        dst[2] = '-';
        n = 2u;
    }
    dst[n + 1u] = '\0';
}

static const char *direction_text(const char *direction)
{
    if (direction && strcmp(direction, "\xC9\xCF\xD0\xD0") == 0)
        return "\xC9\xCF\xD0\xD0"; /* 上行 */
    if (direction && strcmp(direction, "\xCF\xC2\xD0\xD0") == 0)
        return "\xCF\xC2\xD0\xD0"; /* 下行 */
    return "\xB4\xFD\xB6\xA8"; /* 待定 */
}

static void format_main_header(char *dst, size_t cap, const LBJ_TrainRecord *r)
{
    const char *train_no = (r->train_no[0] != '\0') ? r->train_no : "--";
    const char *direction = direction_text(r->direction);
    char speed[12] = "";
    int written;

    if (r->speed_valid)
        (void)snprintf(speed, sizeof(speed), "%s", r->speed);

    /* Keep everything on one line. First try units; drop only "km/h" if needed. */
    written = snprintf(dst, cap, "\xB3\xB5%s %s%s%s", train_no, direction,
                       speed, r->speed_valid ? "km/h" : "");
    if (written < 0 || (size_t)written >= cap)
        dst[0] = '\0';

    if (dst[0] && text_pixel_width(dst) > 127u && r->speed_valid) {
        written = snprintf(dst, cap, "\xB3\xB5%s %s%s", train_no, direction, speed);
        if (written < 0 || (size_t)written >= cap)
            dst[0] = '\0';
    }

    /* This fallback is only for unusually long train numbers. */
    if (!dst[0] || text_pixel_width(dst) > 127u) {
        written = snprintf(dst, cap, "%s%s%s", train_no, direction,
                           r->speed_valid ? speed : "");
        if (written < 0 || (size_t)written >= cap)
            dst[0] = '\0';
    }
}

static void draw_status(uint8_t selected, uint8_t count, uint8_t page)
{
    char status[20];
    (void)snprintf(status, sizeof(status), "%02u/%02u %u/2",
                   (unsigned)(selected + 1u), (unsigned)count,
                   (unsigned)page);
    GUI_DisplaySmallest(status, 1u, 1u, true, true);
}

void UI_DisplayLBJ(void)
{
    const LBJ_Engine *engine = LBJ_AppGetEngine();
    const LBJ_TrainRecord *r = LBJ_EngineSelected(engine);
    char line[LBJ_TEXT_TYPE_MAX + 32u];
    char model[32];
    char km_display[8];

    UI_DisplayClear();
    memset(gStatusLine, 0, sizeof(gStatusLine));

    if (LBJ_AppBootSplashActive()) {
        /* One-time startup identity; the frequency is not kept on the main screen. */
        print_line("LBJ RX 821.2375", 1u);
        print_line("\xCC\xFA\xC2\xB7\xC1\xD0\xB3\xB5\xD4\xA4\xBE\xAF\xBD\xD3\xCA\xD5", 3u);
        print_line("POCSAG 1200", 5u);
    } else if (!r) {
        print_line("\xB5\xC8\xB4\xFD LBJ POCSAG", 0u);
        print_line("\xB5\xC8\xB4\xFD\xC1\xD0\xB3\xB5\xB1\xA8\xCE\xC4", 2u);
        GUI_DisplaySmallest("UP/DN HISTORY", 15u, 48u, false, true);
        draw_status(engine->selected, engine->history_count, 1u);
    } else if (engine->detail_page == 0u) {
        const char *loco_type = r->loco_type[0] ? r->loco_type : "--";
        const char *loco_number = r->loco_number[0] ? r->loco_number : "";
        const char *route = r->route[0] ? r->route : "--";

        format_main_header(line, sizeof(line), r);
        print_line(line, 0u);

        format_km_display(km_display, sizeof(km_display), r->km_valid ? r->km : NULL);
        print_line(km_display, 2u);

        if (loco_number[0])
            (void)snprintf(model, sizeof(model), "\xD0\xCD %s-%s", loco_type, loco_number);
        else
            (void)snprintf(model, sizeof(model), "\xD0\xCD %s", loco_type);
        print_line(model, 3u);

        (void)snprintf(line, sizeof(line), "\xCF\xDF %s", route);
        print_line(line, 5u);
        draw_status(engine->selected, engine->history_count, 1u);
    } else {
        (void)snprintf(line, sizeof(line), "\xC0\xE0 %s", r->train_type[0] ? r->train_type : "--");
        print_line(line, 0u);

        (void)snprintf(line, sizeof(line), "LON %s", r->longitude[0] ? r->longitude : "--");
        print_line(line, 2u);
        (void)snprintf(line, sizeof(line), "LAT %s", r->latitude[0] ? r->latitude : "--");
        print_line(line, 4u);
        draw_status(engine->selected, engine->history_count, 2u);
    }

    ST7565_BlitStatusLine();
    ST7565_BlitFullScreen();
}
