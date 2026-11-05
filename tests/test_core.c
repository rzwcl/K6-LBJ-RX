#include "lbj_core.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void make_nibble_payload(char *out, size_t cap)
{
    static const uint8_t prefix_loco[] = {
        4, 11, 3, 1,              /* ASCII "K1" */
        2, 4, 2, 0, 0, 4, 7, 7,  /* 24200477 */
        3, 1,                    /* A end */
        12, 4, 12, 15, 12, 0, 10, 5, 11, 15, 12, 13, 13, 7, 10, 8, /* GB2312: 南昆客专 */
        1, 0, 4, 1, 2, 3, 4, 5, 6, /* 104123456 */
        2, 5, 1, 2, 3, 4, 5, 6    /* 25123456 */
    };
    static const char map[] = "0123456789.U -)[";
    size_t i, n = 0;
    const char prefix[] = "1234002";
    for (i = 0; prefix[i] && n + 1u < cap; ++i) out[n++] = prefix[i];
    for (i = 0; i < sizeof(prefix_loco) && n + 1u < cap; ++i) {
        uint8_t v = prefix_loco[i];
        out[n++] = map[v];
    }
    out[n] = '\0';
}

static void test_core_binding(void)
{
    LBJ_Engine e;
    LBJ_PocsagPage train;
    LBJ_PocsagPage loco;
    const LBJ_TrainRecord *r;
    char loco_payload[80];

    LBJ_EngineInit(&e);
    memset(&train, 0, sizeof(train));
    train.ric = 1234000u;
    train.received_ms = 1000u;
    train.function = 3u;
    strcpy(train.text, "1234000 206 81 2018");
    train.length = (uint8_t)strlen(train.text);
    LBJ_EngineOnPage(&train, &e);

    memset(&loco, 0, sizeof(loco));
    loco.ric = 1234002u;
    loco.received_ms = 1800u;
    loco.function = 0u;
    make_nibble_payload(loco_payload, sizeof(loco_payload));
    strcpy(loco.text, loco_payload);
    loco.length = (uint8_t)strlen(loco.text);
    LBJ_EngineOnPage(&loco, &e);

    r = LBJ_EngineAt(&e, 0u);
    assert(r != 0);
    assert(strcmp(r->train_no, "K1206") == 0);
    assert(strcmp(r->speed, "81") == 0);
    assert(strcmp(r->km, "201.8") == 0);
    assert(strcmp(r->loco_type, "HXD3D") == 0);
    assert(strcmp(r->loco_number, "00477") == 0);
    assert(strcmp(r->route, "\xC4\xCF\xC0\xA5\xBF\xCD\xD7\xA8") == 0);
    assert(r->bound == 1u);
    assert(strcmp(r->train_type, "\xBF\xEC\xCB\xD9\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5") == 0);
    assert(strcmp(r->direction, "\xC9\xCF\xD0\xD0") == 0);

    /* A repeated 1234000 event inherits the recent 1234002 binding. */
    train.received_ms = 3000u;
    strcpy(train.text, "1234000 206 75 2010");
    train.length = (uint8_t)strlen(train.text);
    LBJ_EngineOnPage(&train, &e);
    r = LBJ_EngineAt(&e, 0u);
    assert(strcmp(r->train_no, "K1206") == 0);
    assert(strcmp(r->loco_number, "00477") == 0);
    assert(strcmp(r->km, "201.0") == 0);
    assert(r->bound == 1u);
    assert(strcmp(r->direction, "\xC9\xCF\xD0\xD0") == 0); /* decreasing km => up */
    assert(e.history_count == 2u);
    LBJ_EngineMove(&e, +1);
    assert(LBJ_EngineSelected(&e) == LBJ_EngineAt(&e, 1u));
    LBJ_EngineToggleDetail(&e);
    assert(e.detail_page == 1u);
    LBJ_EngineToggleDetail(&e);
    assert(e.detail_page == 0u);
}

int main(void)
{
    test_core_binding();
    puts("core tests: PASS");
    return 0;
}
