#ifndef _LBJ_FREESTANDING_STDIO_H
#define _LBJ_FREESTANDING_STDIO_H
#include <stddef.h>
int snprintf(char *restrict s, size_t n, const char *restrict fmt, ...);
int sprintf(char *restrict s, const char *restrict fmt, ...);
#endif
