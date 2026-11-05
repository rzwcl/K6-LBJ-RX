#include "lbj_core.h"
#include "lbj_loco.h"
#include "lbj_train_type.h"

#include <stdio.h>
#include <string.h>

#define LBJ_BIND_WINDOW_MS 2500u
#define INVALID_SPEED 0xFFFFu

static uint32_t elapsed_ms(uint32_t a, uint32_t b)
{
    return (a >= b) ? (a - b) : (b - a);
}

static int is_digit(char c)
{
    return c >= '0' && c <= '9';
}

static void copy_text(char *dst, size_t cap, const char *src)
{
    size_t n;
    if (!dst || cap == 0u)
        return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap)
        n = cap - 1u;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void set_direction(char *dst, size_t cap, LBJ_Direction d)
{
    static const char unknown[] = "\xCE\xB4\xD6\xAA";
    static const char up[] = "\xC9\xCF\xD0\xD0";
    static const char down[] = "\xCF\xC2\xD0\xD0";
    static const char checking[] = "\xBC\xEC\xB2\xE2\xD6\xD0";
    const char *s = unknown;
    if (d == LBJ_DIRECTION_UP) s = up;
    else if (d == LBJ_DIRECTION_DOWN) s = down;
    else if (d == LBJ_DIRECTION_CHECKING) s = checking;
    copy_text(dst, cap, s);
}

static int token_next(const char **cursor, char *out, size_t cap)
{
    const char *p = *cursor;
    size_t n = 0;
    while (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') ++p;
    if (!*p) { *cursor = p; return 0; }
    while (*p && *p != ' ' && *p != '\t' && *p != '\r' && *p != '\n') {
        if (n + 1u < cap) out[n++] = *p;
        ++p;
    }
    out[n] = '\0';
    *cursor = p;
    return 1;
}

static int starts_with(const char *s, const char *prefix)
{
    return strncmp(s, prefix, strlen(prefix)) == 0;
}

static LBJ_TrainRecord *insert_train(LBJ_Engine *e)
{
    unsigned i;
    for (i = LBJ_HISTORY_CAPACITY - 1u; i > 0u; --i)
        e->history[i] = e->history[i - 1u];
    memset(&e->history[0], 0, sizeof(e->history[0]));
    if (e->history_count < LBJ_HISTORY_CAPACITY)
        ++e->history_count;
    e->selected = 0u;
    return &e->history[0];
}

static LBJ_TrainRecord *find_recent_unbound(LBJ_Engine *e, uint32_t t, uint32_t *best_diff)
{
    uint8_t i;
    LBJ_TrainRecord *best = 0;
    uint32_t delta_best = LBJ_BIND_WINDOW_MS + 1u;
    for (i = 0; i < e->history_count; ++i) {
        LBJ_TrainRecord *r = &e->history[i];
        uint32_t d;
        if (!r->valid || r->bound)
            continue;
        d = elapsed_ms(t, r->received_ms);
        if (d <= LBJ_BIND_WINDOW_MS && d < delta_best) {
            best = r;
            delta_best = d;
        }
    }
    if (best_diff) *best_diff = delta_best;
    return best;
}

static void bind_loco(LBJ_TrainRecord *r, const LBJ_PendingLoco *l)
{
    char full_train[8];
    if (!r || !l || !l->valid)
        return;
    copy_text(r->loco_type, sizeof(r->loco_type), l->loco_type);
    copy_text(r->loco_number, sizeof(r->loco_number), l->loco_number);
    copy_text(r->route, sizeof(r->route), l->route);
    copy_text(r->end_position, sizeof(r->end_position), l->end_position);
    copy_text(r->longitude, sizeof(r->longitude), l->longitude);
    copy_text(r->latitude, sizeof(r->latitude), l->latitude);
    r->end_valid = l->end_position[0] != '\0';
    r->bound = 1u;
    full_train[0] = '\0';
    if (l->prefix[0] && l->prefix[1]) {
        size_t prefix_len = strlen(l->prefix);
        size_t train_len = strlen(r->train_no);
        if (prefix_len + train_len < sizeof(full_train)) {
            memcpy(full_train, l->prefix, prefix_len);
            memcpy(full_train + prefix_len, r->train_no, train_len + 1u);
            copy_text(r->train_no, sizeof(r->train_no), full_train);
            LBJ_IdentifyTrainType(r->train_no, r->train_type, sizeof(r->train_type));
        }
    }
}


static void inherit_loco_binding(LBJ_Engine *e, LBJ_TrainRecord *r, const char *raw_train)
{
    uint8_t i;
    size_t raw_len = strlen(raw_train);
    for (i = 1u; i < e->history_count; ++i) {
        const LBJ_TrainRecord *old = &e->history[i];
        size_t old_len = strlen(old->train_no);
        char combined[8];
        size_t prefix_len;
        if (!old->valid || !old->bound || !old->loco_type[0] || old_len <= raw_len)
            continue;
        if (strcmp(old->train_no + (old_len - raw_len), raw_train) != 0)
            continue;
        prefix_len = old_len - raw_len;
        if (prefix_len + raw_len >= sizeof(combined))
            continue;
        memcpy(combined, old->train_no, prefix_len);
        memcpy(combined + prefix_len, raw_train, raw_len + 1u);
        copy_text(r->train_no, sizeof(r->train_no), combined);
        LBJ_IdentifyTrainType(r->train_no, r->train_type, sizeof(r->train_type));
        copy_text(r->loco_type, sizeof(r->loco_type), old->loco_type);
        copy_text(r->loco_number, sizeof(r->loco_number), old->loco_number);
        copy_text(r->route, sizeof(r->route), old->route);
        copy_text(r->end_position, sizeof(r->end_position), old->end_position);
        copy_text(r->longitude, sizeof(r->longitude), old->longitude);
        copy_text(r->latitude, sizeof(r->latitude), old->latitude);
        r->end_valid = old->end_valid;
        r->bound = 1u;
        break;
    }
}

static void try_pending_loco(LBJ_Engine *e, LBJ_TrainRecord *r)
{
    unsigned i, best = LBJ_PENDING_CAPACITY;
    uint32_t best_delta = LBJ_BIND_WINDOW_MS + 1u;
    for (i = 0; i < LBJ_PENDING_CAPACITY; ++i) {
        LBJ_PendingLoco *l = &e->pending_loco[i];
        uint32_t d;
        if (!l->valid) continue;
        d = elapsed_ms(r->received_ms, l->received_ms);
        if (d <= LBJ_BIND_WINDOW_MS && d < best_delta) {
            best = i;
            best_delta = d;
        }
    }
    if (best < LBJ_PENDING_CAPACITY) {
        bind_loco(r, &e->pending_loco[best]);
        e->pending_loco[best].valid = 0u;
    }
}

static int parse_unsigned_bounded(const char *s, unsigned long limit, unsigned long *value)
{
    unsigned long n = 0u;
    const char *p = s;
    if (!s || !*s || !value) return 0;
    while (*p) {
        unsigned digit;
        if (*p < '0' || *p > '9') return 0;
        digit = (unsigned)(*p - '0');
        if (n > (limit - digit) / 10u) return 0;
        n = n * 10u + digit;
        ++p;
    }
    *value = n;
    return 1;
}

static void parse_train_status(LBJ_Engine *e, const LBJ_PocsagPage *p)
{
    const char *cursor;
    char train[8], speed[8], km[8];
    unsigned long speed_num = INVALID_SPEED;
    unsigned long km_num = 0;
    LBJ_TrainRecord *r;
    LBJ_Direction hint = LBJ_DIRECTION_CHECKING;
    uint8_t i;
    const char *payload = p->text + 7;

    (void)memset(train, 0, sizeof(train));
    (void)memset(speed, 0, sizeof(speed));
    (void)memset(km, 0, sizeof(km));
    cursor = payload;
    if (!token_next(&cursor, train, sizeof(train)) || strlen(train) > 5u)
        return;
    for (i = 0; train[i]; ++i) if (!is_digit(train[i])) return;
    if (!token_next(&cursor, speed, sizeof(speed))) speed[0] = '\0';
    if (!token_next(&cursor, km, sizeof(km))) km[0] = '\0';

    if (speed[0]) {
        if (!parse_unsigned_bounded(speed, 400u, &speed_num))
            speed_num = INVALID_SPEED;
    }
    if (km[0]) {
        if (strlen(km) < 2u || strlen(km) > 5u || !parse_unsigned_bounded(km, 99999u, &km_num))
            km[0] = '\0';
    }

    if (p->function == 3u) hint = LBJ_DIRECTION_UP;
    else if (p->function == 1u) hint = LBJ_DIRECTION_DOWN;

    r = insert_train(e);
    r->valid = 1u;
    r->received_ms = p->received_ms;
    copy_text(r->train_no, sizeof(r->train_no), train);
    LBJ_IdentifyTrainType(train, r->train_type, sizeof(r->train_type));
    inherit_loco_binding(e, r, train);
    if (speed_num != INVALID_SPEED) {
        (void)snprintf(r->speed, sizeof(r->speed), "%lu", speed_num);
        r->speed_valid = 1u;
    }
    if (km[0]) {
        (void)snprintf(r->km, sizeof(r->km), "%u.%u", (unsigned)(km_num / 10u), (unsigned)(km_num % 10u));
        r->km_valid = 1u;
    }
    set_direction(r->direction, sizeof(r->direction), hint);

    /* Prefer measured kilometre-marker movement over the POCSAG function hint. */
    if (r->km_valid) {
        for (i = 1u; i < e->history_count; ++i) {
            LBJ_TrainRecord *old = &e->history[i];
            if (old->valid && old->km_valid && strcmp(old->train_no, r->train_no) == 0) {
                unsigned long current_km = 0u, previous_km = 0u;
                const char *q;
                for (q = r->km; *q; ++q) if (*q >= '0' && *q <= '9') current_km = current_km * 10u + (unsigned)(*q - '0');
                for (q = old->km; *q; ++q) if (*q >= '0' && *q <= '9') previous_km = previous_km * 10u + (unsigned)(*q - '0');
                if (current_km < previous_km) set_direction(r->direction, sizeof(r->direction), LBJ_DIRECTION_UP);
                else if (current_km > previous_km) set_direction(r->direction, sizeof(r->direction), LBJ_DIRECTION_DOWN);
                else copy_text(r->direction, sizeof(r->direction), old->direction);
                break;
            }
        }
    }
    try_pending_loco(e, r);
    ++e->decoded_1234000;
}

static int nibble_value(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c == '.') return 10;
    if (c == 'U' || c == 'u') return 11;
    if (c == ' ') return 12;
    if (c == '-') return 13;
    if (c == ')' || c == ']' || c == 'E' || c == 'e') return 14;
    if (c == '[') return 15;
    if (c == 'C' || c == 'c') return 12;
    if (c == 'A' || c == 'a') return 10;
    if (c == 'D' || c == 'd') return 13;
    return -1;
}

static void parse_loco_info(LBJ_PendingLoco *l, const LBJ_PocsagPage *p)
{
    uint8_t nibbles[64];
    size_t i, n = 0u;
    const char *s = p->text + 7;
    char full[9];
    uint16_t code;
    const char *type;
    uint8_t route_bytes[8];
    size_t route_len = 8u;
    int v;

    memset(nibbles, 0, sizeof(nibbles));
    while (*s && n < sizeof(nibbles)) {
        v = nibble_value(*s++);
        if (v >= 0) nibbles[n++] = (uint8_t)v;
    }
    if (n < 12u) return;
    for (i = 4u; i < 12u; ++i) if (nibbles[i] > 9u) return;

    memset(l, 0, sizeof(*l));
    l->valid = 1u;
    l->received_ms = p->received_ms;
    if (n >= 4u) {
        char a = (char)((nibbles[0] << 4) | nibbles[1]);
        char b = (char)((nibbles[2] << 4) | nibbles[3]);
        if ((a >= 'A' && a <= 'Z') || (a >= 'a' && a <= 'z') || (a >= '0' && a <= '9')) l->prefix[0] = (char)(a >= 'a' && a <= 'z' ? a - 32 : a);
        if ((b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || (b >= '0' && b <= '9')) l->prefix[1] = (char)(b >= 'a' && b <= 'z' ? b - 32 : b);
    }
    for (i = 0; i < 8u; ++i) full[i] = (char)('0' + nibbles[4u + i]);
    full[8] = '\0';
    code = (uint16_t)((nibbles[4] * 100u) + (nibbles[5] * 10u) + nibbles[6]);
    type = LBJ_LocoTypeName(code);
    if (type) copy_text(l->loco_type, sizeof(l->loco_type), type);
    else (void)snprintf(l->loco_type, sizeof(l->loco_type), "???%03u", (unsigned)code);

    if (starts_with(l->loco_type, "CRH")) {
        memcpy(l->loco_number, full + 3, 4u);
        l->loco_number[4] = '\0';
    } else {
        memcpy(l->loco_number, full + 3, 5u);
        l->loco_number[5] = '\0';
    }

    if (n >= 14u) {
        unsigned end_code = (unsigned)(nibbles[12] * 10u + nibbles[13]);
        if (end_code == 31u) copy_text(l->end_position, sizeof(l->end_position), "\x41\xB6\xCB");
        else if (end_code == 32u) copy_text(l->end_position, sizeof(l->end_position), "\x42\xB6\xCB");
        else if (end_code == 30u) copy_text(l->end_position, sizeof(l->end_position), "\xCE\xB4\xD6\xAA\xB6\xCB\xCE\xBB");
        else (void)snprintf(l->end_position, sizeof(l->end_position), "%02u", end_code);
    }
    if (n >= 30u) {
        for (i = 0; i < 8u; ++i) route_bytes[i] = (uint8_t)((nibbles[14u + i * 2u] << 4) | nibbles[15u + i * 2u]);
        while (route_len && (route_bytes[route_len - 1u] == 0u || route_bytes[route_len - 1u] == ' ')) --route_len;
        if (route_len >= sizeof(l->route)) route_len = sizeof(l->route) - 1u;
        memcpy(l->route, route_bytes, route_len);
        l->route[route_len] = '\0';
    }
    if (n >= 39u) {
        for (i = 0; i < 9u; ++i) l->longitude[i] = (char)('0' + nibbles[30u + i]);
        l->longitude[9] = '\0';
    }
    if (n >= 47u) {
        for (i = 0; i < 8u; ++i) l->latitude[i] = (char)('0' + nibbles[39u + i]);
        l->latitude[8] = '\0';
    }
}

static void parse_loco_page(LBJ_Engine *e, const LBJ_PocsagPage *p)
{
    LBJ_PendingLoco loco;
    LBJ_TrainRecord *r;
    uint32_t diff;
    unsigned i, slot = 0u;
    parse_loco_info(&loco, p);
    if (!loco.valid) {
        ++e->dropped_bad_pages;
        return;
    }
    r = find_recent_unbound(e, p->received_ms, &diff);
    if (r) {
        bind_loco(r, &loco);
    } else {
        for (i = 0; i < LBJ_PENDING_CAPACITY; ++i) {
            if (!e->pending_loco[i].valid) { slot = i; break; }
            if (e->pending_loco[i].received_ms < e->pending_loco[slot].received_ms) slot = i;
        }
        e->pending_loco[slot] = loco;
    }
    ++e->decoded_1234002;
}

void LBJ_EngineInit(LBJ_Engine *e)
{
    if (e) memset(e, 0, sizeof(*e));
}

void LBJ_EngineOnPage(const LBJ_PocsagPage *p, void *user)
{
    LBJ_Engine *e = (LBJ_Engine *)user;
    if (!e || !p || p->length < 7u) return;
    if (starts_with(p->text, "1234000")) parse_train_status(e, p);
    else if (starts_with(p->text, "1234002")) parse_loco_page(e, p);
}

const LBJ_TrainRecord *LBJ_EngineSelected(const LBJ_Engine *e)
{
    if (!e || e->history_count == 0u || e->selected >= e->history_count) return 0;
    return &e->history[e->selected];
}

const LBJ_TrainRecord *LBJ_EngineAt(const LBJ_Engine *e, uint8_t index)
{
    if (!e || index >= e->history_count) return 0;
    return &e->history[index];
}

void LBJ_EngineMove(LBJ_Engine *e, int direction)
{
    if (!e || e->history_count == 0u) return;
    if (direction < 0 && e->selected > 0u) --e->selected;
    else if (direction > 0 && e->selected + 1u < e->history_count) ++e->selected;
}

void LBJ_EngineToggleDetail(LBJ_Engine *e)
{
    if (e) e->detail_page = (uint8_t)((e->detail_page + 1u) % 2u);
}
