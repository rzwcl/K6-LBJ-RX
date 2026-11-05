#include "lbj_train_type.h"
#include <string.h>
#include <ctype.h>
typedef enum {
    T_UNKNOWN = 0,
    T_POWERED_RETURN,
    T_UNPOWERED_RETURN,
    T_RETURN,
    T_TURNBACK,
    T_Z,
    T_N,
    T_V,
    T_X_NEW,
    T_G_HIGH,
    T_T_EXPRESS,
    T_K_FAST,
    T_L_TEMP,
    T_Y_TOUR,
    T_ORD_FAST,
    T_ORD_FAST_IN,
    T_ORD_SLOW,
    T_ORD_SLOW_IN,
    T_COMMUTE,
    T_EMU,
    T_INTERCITY,
    T_SUBURBAN,
    T_EMU_TEST,
    T_EMU_CONFIRM,
    T_EMU_EXPRESS,
    T_EXPRESS_BULLET,
    T_FAST_INTERMODAL,
    T_FAST_INTERCITY,
    T_FAST_EUROPE,
    T_FAST_CENTRAL_ASIA,
    T_GENERAL_INTERMODAL,
    T_GENERAL_INTERCITY,
    T_GENERAL_EUROPE,
    T_GENERAL_CENTRAL_ASIA,
    T_NO_PREFIX_EMU_FIRE,
    T_NO_PREFIX_EMU_CROSS,
    T_NO_PREFIX_EMU_LOCAL,
    T_COACH_CROSS,
    T_COACH_LOCAL,
    T_FREIGHT_TECH,
    T_FREIGHT_THROUGH,
    T_FREIGHT_SECTION,
    T_PICKUP,
    T_SHUNT,
    T_LOCO_PASS,
    T_LOCO_FREIGHT,
    T_TRIAL,
    T_TRACK,
    T_SERVICE,
    T_RESCUE,
    T_FACTORY,
    T_OVERSIZE,
    T_TEN_THOUSAND,
    T_REFRIGERATED,
    T_EXPRESS_FREIGHT,
    T_COAL,
    T_OIL,
    T_ORIGIN,
    T_EMPTY,
    T_SPECIAL_OVERSIZE,
    T_DISASTER,
    T_SPECIAL,
    T_NEW_SPEED_TRIAL,
    T_HIGH_SPEED_TRIAL,
    T_EXPRESS_TRIAL,
    T_FAST_TRIAL,
    T_DYNAMIC_TRIAL,
} train_type_t;

static const char *const type_names[] = {
    "\xCE\xB4\xD6\xAA",
    "\xD3\xD0\xB6\xAF\xC1\xA6\xBB\xD8\xCB\xCD\xB6\xAF\xB3\xB5\xD7\xE9", /* POWERED_RETURN */
    "\xCE\xDE\xB6\xAF\xC1\xA6\xBB\xD8\xCB\xCD\xB6\xAF\xB3\xB5\xD7\xE9", /* UNPOWERED_RETURN */
    "\xBB\xD8\xCB\xCD\xC1\xD0\xB3\xB5", /* RETURN */
    "\xD5\xDB\xB7\xB5\xC1\xD0\xB3\xB5", /* TURNBACK */
    "\x5A\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* Z */
    "\x4E\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* N */
    "\x56\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* V */
    "\xD0\xC2\xB8\xDF\xCB\xD9\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* X_NEW */
    "\xB8\xDF\xCB\xD9\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* G_HIGH */
    "\xCC\xD8\xBF\xEC\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* T_EXPRESS */
    "\xBF\xEC\xCB\xD9\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* K_FAST */
    "\xC1\xD9\xCA\xB1\xC2\xC3\xBF\xCD\xC1\xD0\xB3\xB5", /* L_TEMP */
    "\xC2\xC3\xD3\xCE\xC1\xD0\xB3\xB5", /* Y_TOUR */
    "\xC6\xD5\xCD\xA8\xBF\xEC\xB3\xB5", /* ORD_FAST */
    "\xC6\xD5\xCD\xA8\xBF\xEC\xB3\xB5\x28\xB9\xDC\xC4\xDA\x29", /* ORD_FAST_IN */
    "\xC6\xD5\xCD\xA8\xC2\xFD\xB3\xB5", /* ORD_SLOW */
    "\xC6\xD5\xCD\xA8\xC2\xFD\xB3\xB5\x28\xB9\xDC\xC4\xDA\x29", /* ORD_SLOW_IN */
    "\xCD\xA8\xC7\xDA\xC1\xD0\xB3\xB5", /* COMMUTE */
    "\xB6\xAF\xB3\xB5\xD7\xE9", /* EMU */
    "\xB3\xC7\xBC\xCA\xB6\xAF\xB3\xB5\xD7\xE9", /* INTERCITY */
    "\xCA\xD0\xBD\xBC\xC1\xD0\xB3\xB5", /* SUBURBAN */
    "\xB6\xAF\xB3\xB5\xD7\xE9\xBC\xEC\xB2\xE2\xC1\xD0\xB3\xB5", /* EMU_TEST */
    "\xB6\xAF\xB3\xB5\xD7\xE9\xC8\xB7\xC8\xCF\xC1\xD0\xB3\xB5", /* EMU_CONFIRM */
    "\xB6\xAF\xB3\xB5\xD7\xE9\xBF\xEC\xD4\xCB\xC1\xD0\xB3\xB5", /* EMU_EXPRESS */
    "\xCC\xD8\xBF\xEC\xB0\xE0\xC1\xD0", /* EXPRESS_BULLET */
    "\xBF\xEC\xCB\xD9\xB6\xE0\xCA\xBD\xC1\xAA\xD4\xCB\xB0\xE0\xC1\xD0", /* FAST_INTERMODAL */
    "\xBF\xEC\xCB\xD9\xB3\xC7\xBC\xCA\xB0\xE0\xC1\xD0", /* FAST_INTERCITY */
    "\xBF\xEC\xCB\xD9\xD6\xD0\xC5\xB7\xB0\xE0\xC1\xD0", /* FAST_EUROPE */
    "\xBF\xEC\xCB\xD9\xD6\xD0\xD1\xC7\xB0\xE0\xC1\xD0", /* FAST_CENTRAL_ASIA */
    "\xC6\xD5\xBF\xEC\xB6\xE0\xCA\xBD\xC1\xAA\xD4\xCB\xB0\xE0\xC1\xD0", /* GENERAL_INTERMODAL */
    "\xC6\xD5\xBF\xEC\xB3\xC7\xBC\xCA\xB0\xE0\xC1\xD0", /* GENERAL_INTERCITY */
    "\xC6\xD5\xBF\xEC\xD6\xD0\xC5\xB7\xB0\xE0\xC1\xD0", /* GENERAL_EUROPE */
    "\xC6\xD5\xBF\xEC\xD6\xD0\xD1\xC7\xB0\xE0\xC1\xD0", /* GENERAL_CENTRAL_ASIA */
    "\xB6\xAF\xB3\xB5\xD7\xE9\xD3\xD0\xBB\xF0\xBB\xD8\xCB\xCD", /* NO_PREFIX_EMU_FIRE */
    "\xB6\xAF\xB3\xB5\xD7\xE9\xCE\xDE\xBB\xF0\xBF\xE7\xBE\xD6\xBB\xD8\xCB\xCD", /* NO_PREFIX_EMU_CROSS */
    "\xB6\xAF\xB3\xB5\xD7\xE9\xCE\xDE\xBB\xF0\xB9\xDC\xC4\xDA\xBB\xD8\xCB\xCD", /* NO_PREFIX_EMU_LOCAL */
    "\xBF\xE7\xBE\xD6\xBB\xD8\xCB\xCD\xBF\xCD\xB3\xB5", /* COACH_CROSS */
    "\xB9\xDC\xC4\xDA\xBB\xD8\xCB\xCD\xBF\xCD\xB3\xB5", /* COACH_LOCAL */
    "\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5\x20\x28\xBC\xBC\xCA\xF5\xD6\xB1\xB4\xEF\x29", /* FREIGHT_TECH */
    "\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5\x20\x28\xD6\xB1\xCD\xA8\x29", /* FREIGHT_THROUGH */
    "\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5\x20\x28\xC7\xF8\xB6\xCE\x29", /* FREIGHT_SECTION */
    "\xD5\xAA\xB9\xD2\xC1\xD0\xB3\xB5", /* PICKUP */
    "\xD0\xA1\xD4\xCB\xD7\xAA\xC1\xD0\xB3\xB5", /* SHUNT */
    "\xBF\xCD\xB3\xB5\xB5\xA5\xBB\xFA", /* LOCO_PASS */
    "\xBB\xF5\xB3\xB5\xB5\xA5\xBB\xFA", /* LOCO_FREIGHT */
    "\xCA\xD4\xD4\xCB\xD0\xD0\xC1\xD0\xB3\xB5", /* TRIAL */
    "\xB9\xEC\xB5\xC0\xB3\xB5\x2F\xD0\xA1\xD0\xCD\xB9\xA4\xB3\xCC\xB3\xB5", /* TRACK */
    "\xC2\xB7\xD3\xC3\xC1\xD0\xB3\xB5", /* SERVICE */
    "\xBE\xC8\xD4\xAE\xC1\xD0\xB3\xB5", /* RESCUE */
    "\xB9\xA4\xB3\xA7\xD7\xD4\xB1\xB8\xB3\xB5", /* FACTORY */
    "\xB3\xAC\xCF\xDE\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5", /* OVERSIZE */
    "\xCD\xF2\xB6\xD6\xBB\xF5\xCE\xEF\xC1\xD0\xB3\xB5", /* TEN_THOUSAND */
    "\xC0\xE4\xB2\xD8\xC1\xD0\xB3\xB5", /* REFRIGERATED */
    "\xBF\xEC\xD4\xCB\xBB\xF5\xCE\xEF\xC1\xD0\xB3\xB5", /* EXPRESS_FREIGHT */
    "\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5\x20\x28\xC3\xBA\xCC\xBF\xD6\xB1\xB4\xEF\x29", /* COAL */
    "\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5\x20\x28\xCA\xAF\xD3\xCD\xD6\xB1\xB4\xEF\x29", /* OIL */
    "\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5\x20\x28\xCA\xBC\xB7\xA2\xD6\xB1\xB4\xEF\x29", /* ORIGIN */
    "\xBB\xF5\xD4\xCB\xC1\xD0\xB3\xB5\x20\x28\xBF\xD5\xB3\xB5\xD6\xB1\xB4\xEF\x29", /* EMPTY */
    "\xCC\xD8\xCA\xE2\xB3\xAC\xCF\xDE", /* SPECIAL_OVERSIZE */
    "\xC7\xC0\xCF\xD5\xBE\xC8\xD4\xD6\xC1\xD0\xB3\xB5", /* DISASTER */
    "\xCC\xD8\xD6\xD6\xC1\xD0\xB3\xB5", /* SPECIAL */
    "\xD0\xC2\xB8\xDF\xCB\xD9\xCA\xD4\xD4\xCB\xD0\xD0\xC1\xD0\xB3\xB5", /* NEW_SPEED_TRIAL */
    "\xB8\xDF\xCB\xD9\xCA\xD4\xD4\xCB\xD0\xD0\xC1\xD0\xB3\xB5", /* HIGH_SPEED_TRIAL */
    "\xCC\xD8\xBF\xEC\xCA\xD4\xD4\xCB\xD0\xD0\xC1\xD0\xB3\xB5", /* EXPRESS_TRIAL */
    "\xBF\xEC\xCB\xD9\xCA\xD4\xD4\xCB\xD0\xD0\xC1\xD0\xB3\xB5", /* FAST_TRIAL */
    "\xB6\xAF\xBC\xAF\xCA\xD4\xD4\xCB\xD0\xD0\xC1\xD0\xB3\xB5", /* DYNAMIC_TRIAL */
};

static train_type_t numeric_type(const char *digits)
{
    unsigned long n = 0;
    size_t len, i;
    if (!digits || !*digits) return T_UNKNOWN;
    len = strlen(digits);
    for (i = 0; i < len; ++i) { if (digits[i] < '0' || digits[i] > '9') return T_UNKNOWN; n = n * 10u + (unsigned)(digits[i] - '0'); if (n > 999999u) return T_UNKNOWN; }
    if (len > 5u) return T_UNKNOWN;
    if (n >= 1u && n <= 100u) return T_NO_PREFIX_EMU_FIRE;
    if (n >= 101u && n <= 198u) return T_NO_PREFIX_EMU_CROSS;
    if (n >= 201u && n <= 298u) return T_NO_PREFIX_EMU_LOCAL;
    if (n >= 301u && n <= 398u) return T_COACH_CROSS;
    if (n >= 401u && n <= 498u) return T_COACH_LOCAL;
    if (n >= 1001u && n <= 3998u) return T_ORD_FAST;
    if (n >= 4001u && n <= 5998u) return T_ORD_FAST_IN;
    if (n >= 6001u && n <= 6198u) return T_ORD_SLOW;
    if (n >= 6201u && n <= 7598u) return T_ORD_SLOW_IN;
    if (n >= 7601u && n <= 8998u) return T_COMMUTE;
    if (n >= 10001u && n <= 19998u) return T_FREIGHT_TECH;
    if (n >= 20001u && n <= 29998u) return T_FREIGHT_THROUGH;
    if (n >= 30001u && n <= 39998u) return T_FREIGHT_SECTION;
    if (n >= 40001u && n <= 44998u) return T_PICKUP;
    if (n >= 45001u && n <= 49998u) return T_SHUNT;
    if (n >= 50001u && n <= 51998u) return T_LOCO_PASS;
    if (n >= 52001u && n <= 54998u) return T_LOCO_FREIGHT;
    if (n >= 55001u && n <= 55998u) return T_TRIAL;
    if (n >= 56001u && n <= 56998u) return T_TRACK;
    if (n >= 57001u && n <= 57998u) return T_SERVICE;
    if (n >= 59001u && n <= 59998u) return T_RESCUE;
    if (n >= 60001u && n <= 69998u) return T_FACTORY;
    if (n >= 70001u && n <= 70998u) return T_OVERSIZE;
    if (n >= 71001u && n <= 77998u) return T_TEN_THOUSAND;
    if (n >= 78001u && n <= 78998u) return T_REFRIGERATED;
    if (n >= 79001u && n <= 79998u) return T_EXPRESS_FREIGHT;
    if (n >= 80001u && n <= 84998u) return T_COAL;
    if (n >= 85001u && n <= 85998u) return T_OIL;
    if (n >= 86001u && n <= 86998u) return T_ORIGIN;
    if (n >= 87001u && n <= 89998u) return T_EMPTY;
    if (n >= 90001u && n <= 91998u) return T_SPECIAL_OVERSIZE;
    if (n >= 93001u && n <= 94998u) return T_SPECIAL_OVERSIZE;
    if (n >= 95001u && n <= 95998u) return T_DISASTER;
    if (n >= 98001u && n <= 99998u) return T_SPECIAL;
    return T_UNKNOWN;
}

static train_type_t identify(const char *no, unsigned depth)
{
    char digits[16]; size_t len, i; unsigned long n = 0; train_type_t base;
    if (!no || !*no || depth > 4u) return T_UNKNOWN;
    len = strlen(no); if (len >= sizeof(digits)) return T_UNKNOWN;
    for (i = 0; i < len; ++i) { char c = (char)toupper((unsigned char)no[i]); digits[i] = c; } digits[len] = 0;
    if (digits[0] == 'F') {
        base = identify(digits + 1, depth + 1u);
        switch (base) {
          case T_Z: return T_TURNBACK;
          case T_N: return T_TURNBACK;
          case T_V: return T_TURNBACK;
          case T_X_NEW: return T_TURNBACK;
          case T_G_HIGH: return T_TURNBACK;
          case T_T_EXPRESS: return T_TURNBACK;
          case T_K_FAST: return T_TURNBACK;
          case T_L_TEMP: return T_TURNBACK;
          case T_Y_TOUR: return T_TURNBACK;
          case T_ORD_FAST: return T_TURNBACK;
          case T_ORD_FAST_IN: return T_TURNBACK;
          case T_ORD_SLOW: return T_TURNBACK;
          case T_ORD_SLOW_IN: return T_TURNBACK;
          case T_COMMUTE: return T_TURNBACK;
          case T_EMU: return T_TURNBACK;
          case T_INTERCITY: return T_TURNBACK;
          case T_SUBURBAN: return T_TURNBACK;
          default: return base == T_UNKNOWN ? T_TURNBACK : base;
        }
    }
    if (digits[0] == '0' && len >= 2u && digits[1] == '0') {
        if (len == 2u) return T_UNKNOWN;
        for (i = 2; i < len; ++i) { if (digits[i] < '0' || digits[i] > '9') return T_UNKNOWN; n = n * 10u + (unsigned)(digits[i] - '0'); }
        if (n >= 1u && n <= 498u) return T_POWERED_RETURN;
        if (n >= 501u && n <= 698u) return T_UNPOWERED_RETURN;
        return numeric_type(digits);
    }
    if (digits[0] == '0') { base = identify(digits + 1, depth + 1u); return base == T_UNKNOWN ? T_RETURN : base; }
    if (strncmp(digits, "DJ", 2u) == 0) {
        const char *p = digits + 2; if (!*p) return T_UNKNOWN;
        for (i=0; p[i]; ++i) { if (p[i]<'0'||p[i]>'9') return T_UNKNOWN; n=n*10u+(unsigned)(p[i]-'0'); }
        if (n >= 1u && n <= 1998u) return T_EMU_TEST;
        if (n >= 2001u && n <= 9998u) return T_EMU_CONFIRM;
        return numeric_type(p);
    }
    {
        const char *p = digits; unsigned prefix = 0;
        if (strncmp(p,"DH",2u)==0) { prefix=1u; p+=2; }
        else if (strchr("CZT KLYXDGNSVB", p[0]) && p[0] != ' ') { prefix=(unsigned char)p[0]; p++; }
        if (!*p) return T_UNKNOWN;
        n=0; for (i=0;p[i];++i) { if(p[i]<'0'||p[i]>'9') return T_UNKNOWN; n=n*10u+(unsigned)(p[i]-'0'); if(n>999999u)return T_UNKNOWN; }
        if ((prefix=='X'||prefix=='G'||prefix=='T'||prefix=='K'||prefix=='D') && n>=55001u && n<=55998u) {
            if (prefix=='X') return T_NEW_SPEED_TRIAL;
            if (prefix=='G') return T_HIGH_SPEED_TRIAL;
            if (prefix=='T') return T_EXPRESS_TRIAL;
            if (prefix=='K') return T_FAST_TRIAL;
            return T_DYNAMIC_TRIAL;
        }
        if(prefix==1u) return n>=1u&&n<=1998u?T_EMU_EXPRESS:numeric_type(p);
        if(prefix=='B') {
            if(n>=1u&&n<=198u)return T_EXPRESS_BULLET;
            if(n>=201u&&n<=998u)return T_FAST_INTERMODAL;
            if(n>=1001u&&n<=1998u)return T_FAST_INTERCITY;
            if(n>=2001u&&n<=2998u)return T_FAST_EUROPE;
            if(n>=3001u&&n<=3998u)return T_FAST_CENTRAL_ASIA;
            if(n>=4001u&&n<=5998u)return T_GENERAL_INTERMODAL;
            if(n>=6001u&&n<=7998u)return T_GENERAL_INTERCITY;
            if(n>=8001u&&n<=8998u)return T_GENERAL_EUROPE;
            if(n>=9001u&&n<=9998u)return T_GENERAL_CENTRAL_ASIA;
            return numeric_type(p); }
        if(prefix=='Z') return (n>=1u&&n<=1998u)?T_Z:numeric_type(p);
        if(prefix=='N') return (n>=1u&&n<=1998u)?T_N:numeric_type(p);
        if(prefix=='V') return (n>=1u&&n<=7998u)?T_V:numeric_type(p);
        if(prefix=='T') return (n>=1u&&n<=9998u)?T_T_EXPRESS:numeric_type(p);
        if(prefix=='K') return (n>=1u&&n<=9998u)?T_K_FAST:numeric_type(p);
        if(prefix=='Y') return ((n>=1u&&n<=2998u)||(n>=5001u&&n<=9998u))?T_Y_TOUR:numeric_type(p);
        if(prefix=='C') return (n>=1u&&n<=2998u)?T_INTERCITY:numeric_type(p);
        if(prefix=='D') return (n>=1u&&n<=9998u)?T_EMU:numeric_type(p);
        if(prefix=='G') return (n>=1u&&n<=9998u)?T_G_HIGH:numeric_type(p);
        if(prefix=='S') return (n>=1u&&n<=2998u)?T_SUBURBAN:numeric_type(p);
        if(prefix=='L') return (n>=1u&&n<=9998u)?T_L_TEMP:numeric_type(p);
        if(prefix=='X') return (n>=1u&&n<=9998u)?T_X_NEW:numeric_type(p);
        return numeric_type(digits);
    }
}

void LBJ_IdentifyTrainType(const char *train_no, char *out, size_t out_size)
{
    train_type_t t; const char *name; size_t n;
    if (!out || out_size == 0u)
        return;
    out[0] = 0;
    t=identify(train_no,0u);
    if ((unsigned)t >= sizeof(type_names)/sizeof(type_names[0])) t=T_UNKNOWN;
    name=type_names[(unsigned)t]; n=strlen(name); if(n>=out_size)n=out_size-1u; memcpy(out,name,n); out[n]=0;
    if (train_no && train_no[0]=='0' && train_no[1]!='0' && t!=T_RETURN) { size_t base=n; static const char suffix[]="(\xBB\xD8\xCB\xCD)"; if(base+sizeof(suffix)<=out_size) memcpy(out+base,suffix,sizeof(suffix)); }
}
