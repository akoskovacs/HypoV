/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Character display implementation                           |
 * +------------------------------------------------------------+
*/
#include <types.h>
#include <string.h>
#include <char_display.h>

struct CharacterDisplay *stdout = NULL;

#define CDISP_CHECK_THIS(this)       \
    if ((this == NULL)) {            \
        return -HV_ENODISP;    \
    }                          

#define CDISP_CALL_FUNCTION(fname, ...)        \

int hv_disp_setup(struct CharacterDisplay *this)
{
    CDISP_CHECK_THIS(this);
    if (this->disp_setup) {
        return this->disp_setup(this);
    }
    return -HV_ENOIMPL;
}

int hv_disp_clear(struct CharacterDisplay *this)
{
    CDISP_CHECK_THIS(this);
    if (this->disp_clear) {
        return this->disp_clear(this);
    }
    return -HV_ENOIMPL;
}

int hv_disp_get_xy(struct CharacterDisplay *this, int *x, int *y)
{
    CDISP_CHECK_THIS(this);
    if (this->disp_get_xy) {
        return this->disp_get_xy(this, x, y);
    }
    return -HV_ENOIMPL;
}

int hv_disp_get_max_xy(struct CharacterDisplay *this, int *x, int *y)
{
    CDISP_CHECK_THIS(this);
    if (this->disp_get_max_xy) {
        return this->disp_get_max_xy(this, x, y);
    }
    return -HV_ENOIMPL;
}

int hv_disp_putc(struct CharacterDisplay *this, char ch)
{
    CDISP_CHECK_THIS(this);
    if (this->disp_putc) {
        return this->disp_putc(this, ch);
    }
    return -HV_ENOIMPL;
}

int hv_disp_putc_xy(struct CharacterDisplay *this, int x, int y, char ch)
{
    CDISP_CHECK_THIS(this);
    if (this->disp_putc_xy) {
        return this->disp_putc_xy(this, x, y, ch);
    }
    return -HV_ENOIMPL;
}

int hv_disp_puts_xy(struct CharacterDisplay *this, int x, int y, const char *line)
{
    int i, r = 0;
    size_t size = strlen(line);

    CDISP_CHECK_THIS(this);
    if (this->disp_putc_xy == NULL) {
        return -HV_ENOIMPL;
    }

    for (i = 0; i < size; i++) {
        // NOTE: No out of screen handling
        r = this->disp_putc_xy(this, x+1, y, line[i]);
        if (r < 0) {
            return r;
        }
    }
    return size;
}

int hv_disp_puts(struct CharacterDisplay *this, const char *line)
{
    int i, r = 0;
    size_t size = strlen(line);
    CDISP_CHECK_THIS(this);
    if (this->disp_putc_xy == NULL) {
        return -HV_ENOIMPL;
    }
    for (i = 0; i < size; i++) {
        r = this->disp_putc(this, line[i]);
        if (r < 0) {
            return r;
        }
    }
    return size;
}
