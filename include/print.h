/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Generic printf() functions                                 |
 * +------------------------------------------------------------+
*/
#ifndef PRINT_H
#define PRINT_H

#include <basic.h>
#include <types.h>
#include <char_display.h>

int hv_vsnprintf(char *dest, size_t size, const char *fmt, va_list ap);
int hv_snprintf(char *dest, size_t size, const char *fmt, ...);

int hv_printf(struct CharacterDisplay *disp, const char *fmt, ...);
int hv_printf_xy(struct CharacterDisplay *disp, int x, int y, const char *fmt, ...);

#endif // PRINT_H