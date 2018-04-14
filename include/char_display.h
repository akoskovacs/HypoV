/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Base stuctures and functions for character displays        |
 * +------------------------------------------------------------+
*/
#ifndef CHAR_DISPLAY_H
#define CHAR_DISPLAY_H

#include <basic.h>
#include <error.h>

struct CharacterDisplay;

/* Function typedefs for each callback handler */
typedef int (*hv_disp_setup_ft)(struct CharacterDisplay *);
typedef int (*hv_disp_clear_ft)(struct CharacterDisplay *);
typedef int (*hv_disp_get_xy_ft)(struct CharacterDisplay *, int *x, int *y);
typedef int (*hv_disp_putc_ft)(struct CharacterDisplay *, char c);
typedef int (*hv_disp_putc_xy_ft)(struct CharacterDisplay *, int x, int y, char c);

/* 
 * Must be encapsulated as the first variable in each implementor
 * structure
*/
struct CharacterDisplay {
    const char         *disp_name;
    hv_disp_setup_ft   disp_setup;
    hv_disp_clear_ft   disp_clear;
    hv_disp_get_xy_ft  disp_get_max_xy;
    hv_disp_get_xy_ft  disp_get_xy;
    hv_disp_putc_xy_ft disp_putc_xy;
    hv_disp_putc_ft    disp_putc;
};

extern struct CharacterDisplay *hv_stdout;

/* Top-level functions */
int hv_disp_setup(struct CharacterDisplay *this);
int hv_disp_clear(struct CharacterDisplay *this);
int hv_disp_get_xy(struct CharacterDisplay *this, int *x, int *y);
int hv_disp_get_max_xy(struct CharacterDisplay *this, int *x, int *y);
int hv_disp_putc(struct CharacterDisplay *this, char ch);
int hv_disp_putc_xy(struct CharacterDisplay *this, int x, int y, char ch);
int hv_disp_puts_xy(struct CharacterDisplay *this, int x, int y, const char *line);
int hv_disp_puts(struct CharacterDisplay *this, const char *line);

#define putchar(ch)         hv_disp_putc(stdout, (ch))
#define puts(str)           hv_disp_puts(stdout, (str))
#define fputc(ch, disp)     hv_disp_putc((disp), (ch))
#define fputs(str, disp)    hv_disp_puts((disp), (str))
#define hv_set_stdout(disp) (hv_stdout = (struct CharacterDisplay *)(disp))

#endif // CHAR_DISPLAY_H