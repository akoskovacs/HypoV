/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Display driver for the Debug Console                       |
 * +------------------------------------------------------------+
*/

#include <basic.h>
#include <char_display.h>
#include <drivers/video/pc_console.h>
#include <system.h>
#include <string.h>
#include <types.h>

int hv_console_display_init(struct ConsoleDisplay *m) {
    if (m == NULL) {
        return -HV_ENODISP;
    }

    m->pv_base.disp_name        = CONSOLE_DEVICE_NAME;
    m->pv_base.disp_setup       = hv_console_setup;
    m->pv_base.disp_clear       = hv_console_clear;
    m->pv_base.disp_putc_xy     = hv_console_putc_xy;
    m->pv_base.disp_putc        = hv_console_putc;
    m->pv_base.disp_get_xy      = hv_console_get_xy;
    m->pv_base.disp_get_max_xy  = hv_console_get_max_xy;

    m->pv_attr      = (FG_COLOR_WHITE | LIGHT | BG_COLOR_RED);
    m->pv_tabsize   = CONSOLE_DEFAULT_TABSIZE;
    m->pv_width     = CONFIG_CONSOLE_WIDTH;
    m->pv_height    = CONFIG_CONSOLE_HEIGHT;
    m->pv_base_addr = PC_VIDEORAM_BASE_ADDRESS;
    m->pv_x         = m->pv_y = 0;

    return 0;
}

int hv_console_setup(struct CharacterDisplay *disp)
{
    // disable_cursor();
    return hv_console_clear(disp);
}

int hv_console_clear(struct CharacterDisplay *cdisp)
{
    struct ConsoleDisplay *disp = (struct ConsoleDisplay *)cdisp;
    hv_console_fill_line(disp, 0, 0, disp->pv_width * disp->pv_height);
    return 0;
}


int hv_console_putc_xya(struct CharacterDisplay *cdisp
                    , int x, int y, console_attr_t attr, char ch)
{
    struct ConsoleDisplay *disp = (struct ConsoleDisplay *)cdisp;
    console_font_t font = 0;

    if (x < 0 || y < 0) {
        return -HV_BADARG;
    }

    if (x > CONSOLE_LAST_COLUMN(disp) || y > CONSOLE_LAST_ROW(disp)) {
        return -HV_ETOOLONG;
    }

    switch (ch) {
        case '\b':
            x--;
            ch = ' ';
        break;

        case '\n': case '\r':
        return 0;

        case '\t':
            hv_console_fill_line(disp, x, y, disp->pv_tabsize);
        return 0;
    }

    font |= ch;
    font |= (attr << 8);
    disp->pv_base_addr[y * disp->pv_width + x] = font; // Copy to screen
    return 0;
}

int hv_console_putc_xy(struct CharacterDisplay *cdisp, int x, int y, char ch)
{
    struct ConsoleDisplay *disp = (struct ConsoleDisplay *)cdisp;
    return hv_console_putc_xya(cdisp, x, y, disp->pv_attr, ch);
}

int hv_console_putc(struct CharacterDisplay *cdisp, char ch)
{
    int i;
    int pos;
    struct ConsoleDisplay *disp = (struct ConsoleDisplay *)cdisp;

    switch (ch) {
        case '\b':
            /* Deleting the first character has
            to throw back us to the previous line last character. */
            if (disp->pv_x == 0) {
                disp->pv_x = disp->pv_width;
                disp->pv_y--;
            }
            disp->pv_x--;
            hv_console_putc_xya(cdisp, disp->pv_x, disp->pv_y, disp->pv_attr, ' ');
        return 0;

        case '\n':
            pos = disp->pv_width - disp->pv_x + 1;
            for (i = 0 ; i < pos; i++) {
                hv_console_putc(cdisp, ' ');
            }
        return 0;

        case '\t':
            for (i = 0 ; i < disp->pv_tabsize; i++) {
                hv_console_putc(cdisp, ' ');
            }
        return 0;
    }

    hv_console_putc_xya(cdisp, disp->pv_x, disp->pv_y, disp->pv_attr,  ch);
    if (disp->pv_x > disp->pv_width-1) { // At the end of the line
        disp->pv_x = 0;
        disp->pv_y++;
        if (disp->pv_y > disp->pv_height-1) // At the end of the screen
            hv_console_scroll_up(disp, 1);
    } else {
        disp->pv_x++;
    }
    return 0;
}

void hv_console_scroll_up(struct ConsoleDisplay *this, int count)
{
   int i;
   int wh = this->pv_width * this->pv_height;
   while (count--) {
       for (i = 0; i < wh; i++) {
           this->pv_base_addr[i] = this->pv_base_addr[this->pv_width + i];
       }
   }
   this->pv_y = this->pv_height - 1;
   this->pv_x = 0;
   hv_console_fill_line(this, this->pv_x, this->pv_y, this->pv_width);
}

void hv_console_scroll_down(struct ConsoleDisplay *this, int count)
{
   int i;
   int wh = this->pv_width * this->pv_height;
   while (count--) {
       for (i = wh; i > 0; i++) {
           this->pv_base_addr[this->pv_width - i] = this->pv_base_addr[i];
       }
   }
   this->pv_y = 0;
   this->pv_x = 0;
   hv_console_fill_line(this, this->pv_x, this->pv_y, this->pv_width);
}

int hv_console_get_xy(struct CharacterDisplay *this, int *x, int *y)
{
    struct ConsoleDisplay *disp = (struct ConsoleDisplay *)this;
    int r = (x != NULL) || (x != NULL);
    if (r) {
        if (x != NULL) {
            *x = disp->pv_x;
        }

        if (y != NULL) {
            *y = disp->pv_y;
        }
    } else {
        return -HV_BADARG;
    }
    return 0;
}

int hv_console_get_max_xy(struct CharacterDisplay *this, int *x, int *y)
{
    struct ConsoleDisplay *disp = (struct ConsoleDisplay *)this;
    int r = (x != NULL) || (x != NULL);
    if (r) {
        if (x != NULL) {
            *x = disp->pv_width - 1;
        }

        if (y != NULL) {
            *y = disp->pv_height - 1;
        }
    } else {
        return -HV_BADARG;
    }
    return 0;
}

void hv_console_fill_line(struct ConsoleDisplay *this, int x, int y, int count)
{
   while (count) {
       if (x > this->pv_width - 1) {
           x = 0;
           y++;
       }
       hv_console_putc_xy((struct CharacterDisplay *)this, x, y, ' ');
       x++;
       count--;
   }
}

void hv_console_cursor_disable(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

// TODO
#if 0
void move_console_cursor(int x, int y)
{
   unsigned temp;

   temp = y * CONFIG_CONSOLE_WIDTH + x;

   outb(0x3D4, 14);
   outb(0x3D5, temp >> 8);
   outb(0x3D4, 15);
   outb(0x3D5, temp);
}

void update_cursor(void)
{
   move_console_cursor(pos_x, pos_y);
}

void disable_cursor(void)
{
   outw(0x3D4,0x200A);
   outw(0x3D4,0xB);
}
#endif