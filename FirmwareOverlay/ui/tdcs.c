#include "tdcs.h"

#include "app/tdcs_app.h"
#include "ui/helper.h"
#include "driver/st7565.h"
#include "misc.h"
#include "ui/ui.h"

#include <stdio.h>
#include <string.h>

static void print_line(const char *text, uint8_t line)
{
    if (text != NULL && text[0] != '\0') {
        UI_PrintStringSmall(text, 0u, 127u, line);
    }
}

static void format_locomotive(char *dst, size_t capacity,
                              const TDCS_PayloadFields *fields)
{
    const char *name = NULL;

    if (fields->loco_model == 237u) {
        name = "HXD1C";
    } else if (fields->loco_model == 106u) {
        name = "DF4C";
    }

    if (name != NULL) {
        (void)snprintf(dst, capacity, "%s-%05u", name,
                       (unsigned int)fields->loco_number);
    } else {
        (void)snprintf(dst, capacity, "M%03u-%05u",
                       (unsigned int)fields->loco_model,
                       (unsigned int)fields->loco_number);
    }
}

static void format_mileage(char *dst, size_t capacity,
                           const TDCS_PayloadFields *fields)
{
    const uint32_t km = fields->mileage_m / 1000u;
    const uint32_t metres = fields->mileage_m % 1000u;

    if (!fields->mileage_valid) {
        (void)snprintf(dst, capacity, "KM ---");
        return;
    }

    (void)snprintf(dst, capacity, "%sK%lu+%03lu",
                   fields->mileage_negative ? "-" : "",
                   (unsigned long)km,
                   (unsigned long)metres);
}

static void draw_footer(uint8_t selected, uint8_t count, uint8_t page)
{
    char status[24];

    (void)snprintf(status, sizeof(status), "%02u/%02u  %u/2",
                   (unsigned int)(selected + 1u),
                   (unsigned int)count,
                   (unsigned int)page);
    GUI_DisplaySmallest(status, 1u, 1u, true, true);
}

static void draw_waiting_screen(void)
{
    print_line("TDCS RX 1200/1800", 0u);
    print_line("WAITING FOR DATA", 2u);
    print_line("BCH(26,16) + CRC16", 4u);
    GUI_DisplaySmallest("UP/DN HISTORY", 15u, 48u, false, true);
    draw_footer(0u, 0u, (uint8_t)(TDCS_AppGetDetailPage() + 1u));
}

static void draw_summary(const TDCS_TrainRecord *record)
{
    char line[32];
    char locomotive[24];
    char mileage[24];
    const TDCS_PayloadFields *fields = &record->fields;

    (void)snprintf(line, sizeof(line), "T %lu  SPD %u",
                   (unsigned long)fields->train_number,
                   (unsigned int)fields->speed_kmh);
    print_line(line, 0u);

    format_locomotive(locomotive, sizeof(locomotive), fields);
    (void)snprintf(line, sizeof(line), "L %s", locomotive);
    print_line(line, 2u);

    format_mileage(mileage, sizeof(mileage), fields);
    print_line(mileage, 3u);

    (void)snprintf(line, sizeof(line), "R %u/%u  C %u",
                   (unsigned int)fields->route_number,
                   (unsigned int)fields->actual_route_number,
                   (unsigned int)fields->car_count);
    print_line(line, 5u);
}

static void draw_details(const TDCS_TrainRecord *record)
{
    char line[32];
    const TDCS_PayloadFields *fields = &record->fields;

    (void)snprintf(line, sizeof(line), "EV %02u SIG %u FLAG %02X",
                   (unsigned int)fields->event_code,
                   (unsigned int)fields->signal_type_code,
                   (unsigned int)fields->class_flags);
    print_line(line, 0u);

    (void)snprintf(line, sizeof(line), "W %ut  LEN %u.%u",
                   (unsigned int)fields->weight_t,
                   (unsigned int)(fields->length_tenths / 10u),
                   (unsigned int)(fields->length_tenths % 10u));
    print_line(line, 2u);

    (void)snprintf(line, sizeof(line), "ST %u  ROUTE %u/%u",
                   (unsigned int)fields->station_code,
                   (unsigned int)fields->route_number,
                   (unsigned int)fields->actual_route_number);
    print_line(line, 3u);

    (void)snprintf(line, sizeof(line), "DRV %06lX",
                   (unsigned long)fields->driver_number);
    print_line(line, 5u);
}

void UI_DisplayTDCS(void)
{
    const TDCS_TrainRecord *record = TDCS_AppGetSelected();
    const uint8_t count = TDCS_AppGetHistoryCount();
    const uint8_t selected = TDCS_AppGetSelectedIndex();
    const uint8_t page = TDCS_AppGetDetailPage();

    UI_DisplayClear();
    memset(gStatusLine, 0, sizeof(gStatusLine));

    if (record == NULL) {
        draw_waiting_screen();
    } else if (page == 0u) {
        draw_summary(record);
        draw_footer(selected, count, 1u);
    } else {
        draw_details(record);
        draw_footer(selected, count, 2u);
    }

    ST7565_BlitStatusLine();
    ST7565_BlitFullScreen();
}

void TDCS_ProcessKeys(KEY_Code_t key, bool pressed, bool held)
{
    (void)held;

    if (!pressed) {
        return;
    }

    switch (key) {
    case KEY_UP:
        TDCS_AppMoveSelection(-1);
        gUpdateDisplay = true;
        break;

    case KEY_DOWN:
        TDCS_AppMoveSelection(1);
        gUpdateDisplay = true;
        break;

    case KEY_MENU:
        TDCS_AppToggleDetail();
        gUpdateDisplay = true;
        break;

    case KEY_EXIT:
        gRequestDisplayScreen = DISPLAY_MAIN;
        break;

    default:
        break;
    }
}
