/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | PC character display driver (inherited from CharDisplay)   |
 * +------------------------------------------------------------+
*/
#ifndef PC_CONSOLE_H
#define PC_CONSOLE_H

#include <types.h>
#include <char_display.h>

#define CONFIG_CONSOLE_WIDTH  80
#define CONFIG_CONSOLE_HEIGHT 25
#define CONSOLE_LAST_COLUMN(disp)   ((disp)->pv_width - 1)
#define CONSOLE_LAST_ROW(disp)      ((disp)->pv_height - 1)
#define CONSOLE_WIDTH(disp)         ((disp)->pv_width)
#define CONSOLE_HEIGHT(disp)        ((disp)->pv_height)

#define hv_console_set_x(disp, v)   ((disp)->pv_x = (v))
#define hv_console_set_y(disp, v)   ((disp)->pv_y = (v))

#define hv_console_set_xy(disp, x, y) do { (disp)->pv_x = (x); \
                                           (disp)->pv_y = (y); } while (0)

#define hv_console_set_attribute(disp, attr) \
                                    ((disp)->pv_attr = (attr))
typedef enum {
    LIGHT            = 0x08,
    BLINK            = 0x80,
    BG_COLOR_RED     = 0x40,
    BG_COLOR_GREEN   = 0x20,
    BG_COLOR_BLUE    = 0x10,
    BG_COLOR_CYAN    = BG_COLOR_GREEN | BG_COLOR_BLUE,
    BG_COLOR_MAGENTA = BG_COLOR_RED | BG_COLOR_BLUE,
    BG_COLOR_BROWN   = BG_COLOR_RED | BG_COLOR_GREEN,
    BG_COLOR_WHITE   = BG_COLOR_RED | BG_COLOR_GREEN | BG_COLOR_BLUE,
    BG_COLOR_BLACK   = 0x00,
    FG_COLOR_RED     = 0x04,
    FG_COLOR_GREEN   = 0x02,
    FG_COLOR_BLUE    = 0x01,
    FG_COLOR_CYAN    = FG_COLOR_GREEN | FG_COLOR_BLUE,
    FG_COLOR_MAGENTA = FG_COLOR_RED | FG_COLOR_BLUE,
    FG_COLOR_BROWN   = FG_COLOR_RED | FG_COLOR_GREEN,
    FG_COLOR_WHITE   = FG_COLOR_RED | FG_COLOR_GREEN | FG_COLOR_BLUE,
    FG_COLOR_BLACK   = 0x00
} console_attr_t;

typedef uint16_t console_font_t;

#ifndef CONSOLE_DEVICE_NAME
# define CONSOLE_DEVICE_NAME     "ConsoleDisplay_0"
#endif // CONSOLE_DEVICE_NAME

#ifndef CONSOLE_DEFAULT_TABSIZE
# define CONSOLE_DEFAULT_TABSIZE 4
#endif // CONSOLE_DEFAULT_TABSIZE

#define PC_VIDEORAM_BASE_ADDRESS   (console_font_t *)0xb8000

/* struct VideoMemory : public CharacterDisplay */
struct ConsoleDisplay {
    struct CharacterDisplay  pv_base;
    volatile console_font_t *pv_base_addr;
    console_attr_t           pv_attr;
    short int                pv_tabsize;
    int                      pv_width;
    int                      pv_height;
    int                      pv_x;
    int                      pv_y;
};

int hv_console_display_init(struct ConsoleDisplay *m);

/* Inherited functions from CharacterDisplay */
int hv_console_setup(struct CharacterDisplay *disp);
int hv_console_clear(struct CharacterDisplay *disp);
int hv_console_get_xy(struct CharacterDisplay *disp, int *x, int *y);
int hv_console_get_max_xy(struct CharacterDisplay *disp, int *x, int *y);

int hv_console_putc_xya(struct CharacterDisplay *cdisp, int x, int y, console_attr_t attr, char ch);
int hv_console_putc_xy(struct CharacterDisplay *cdisp, int x, int y, char ch);
int hv_console_putc(struct CharacterDisplay *cdisp, char ch);


/* ConsoleDisplay's own functions */
console_attr_t hv_console_get_attribute(struct ConsoleDisplay *m);
int            hv_console_puts_xya(struct ConsoleDisplay *m, int x, int y, console_attr_t attr, const char *line);
void           hv_console_scroll_up(struct ConsoleDisplay *m, int count);
void           hv_console_scroll_down(struct ConsoleDisplay *m, int count);
void           hv_console_fill_line(struct ConsoleDisplay *m, int x, int y, int count);
void           hv_console_cursor_disable(void);

#endif // PC_CONSOLE_H