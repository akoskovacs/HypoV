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

enum HV_CDISP_ERROR {
    HV_CDISP_ENODISP = 1, // No display, or NULL
    HV_CDISP_ENOIMPL,     // Function not implemented
    HV_CDISP_ETOOLONG     // Coordinates are out of scope
};

/* Function typedefs for each callback handler */
typedef int (*hv_cdisp_setup_ft)(struct CharacterDisplay *);
typedef int (*hv_cdisp_clear_ft)(struct CharacterDisplay *);
typedef int (*hv_cdisp_putc_ft)(struct CharacterDisplay *, char c);
typedef int (*hv_cdisp_putc_xy_ft)(struct CharacterDisplay *, int x, int y, char c);

/* 
 * Must be encapsulated as the first variable in each implementor
 * structure
*/
struct ChacterDisplay {
    const char         *disp_name;
    hv_cdisp_setup_ft   disp_setup;
    hv_cdisp_clear_ft   disp_clear;
    hv_cdisp_putc_xy_ft disp_putc_xy;
    hv_cdisp_putc_ft    disp_putc;
};

struct ChacterDisplay *stdout = NULL;

/* Top-level functions */
hv_cdisp_setup_ft    hv_cdisp_setup;
hv_cdisp_clear_ft    hv_cdisp_clear;
hv_cdisp_putc_ft     hv_cdisp_putc;
hv_cdisp_putc_xy_ft  hv_cdisp_putc_xy;

int hv_cdisp_puts(struct CharacterDisplay *disp, const char *str);

#define putchar(ch)         hv_char_display_putc(stdout, (ch))
#define fputc(ch, disp)     hv_char_display_putc((disp), (ch))
#define fputs(str, disp)    hv_cdisp_puts((disp), (str))

#endif // CHAR_DISPLAY_H