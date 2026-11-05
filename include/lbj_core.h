#ifndef LBJ_CORE_H
#define LBJ_CORE_H

#include "lbj_pocsag.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LBJ_HISTORY_CAPACITY 10u
#define LBJ_PENDING_CAPACITY 3u
#define LBJ_TEXT_TYPE_MAX 48u
#define LBJ_TEXT_ROUTE_MAX 12u

typedef enum {
    LBJ_DIRECTION_UNKNOWN = 0,
    LBJ_DIRECTION_UP,
    LBJ_DIRECTION_DOWN,
    LBJ_DIRECTION_CHECKING
} LBJ_Direction;

typedef struct {
    uint8_t valid;
    uint8_t bound;
    uint8_t speed_valid;
    uint8_t km_valid;
    uint8_t end_valid;
    uint8_t reserved[3];
    uint32_t received_ms;
    char train_no[8];
    char train_type[LBJ_TEXT_TYPE_MAX];
    char speed[4];
    char km[8];
    char direction[8];
    char loco_type[16];
    char loco_number[6];
    char route[LBJ_TEXT_ROUTE_MAX];
    char end_position[8];
    char longitude[10];
    char latitude[9];
} LBJ_TrainRecord;

typedef struct {
    uint8_t valid;
    uint8_t reserved[3];
    uint32_t received_ms;
    char prefix[3];
    char loco_type[16];
    char loco_number[6];
    char route[LBJ_TEXT_ROUTE_MAX];
    char end_position[8];
    char longitude[10];
    char latitude[9];
} LBJ_PendingLoco;

typedef struct {
    LBJ_TrainRecord history[LBJ_HISTORY_CAPACITY];
    uint8_t history_count;
    uint8_t selected;
    uint8_t detail_page;
    uint8_t reserved;
    LBJ_PendingLoco pending_loco[LBJ_PENDING_CAPACITY];
    uint32_t decoded_1234000;
    uint32_t decoded_1234002;
    uint32_t dropped_unmatched_loco;
    uint32_t dropped_bad_pages;
} LBJ_Engine;

void LBJ_EngineInit(LBJ_Engine *engine);
void LBJ_EngineOnPage(const LBJ_PocsagPage *page, void *user);
const LBJ_TrainRecord *LBJ_EngineSelected(const LBJ_Engine *engine);
const LBJ_TrainRecord *LBJ_EngineAt(const LBJ_Engine *engine, uint8_t index);
void LBJ_EngineMove(LBJ_Engine *engine, int direction);
void LBJ_EngineToggleDetail(LBJ_Engine *engine);

#endif
