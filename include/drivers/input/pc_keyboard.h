/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Polling PC keyboard driver                                 |
 * +------------------------------------------------------------+
*/
#ifndef PC_KEYBOARD_H
#define PC_KEYBOARD_H

#include <types.h>

enum KEYBOARD_KEY {
    KEY_ESC         = 0x01,
    KEY_F1          = 0x3B,
    KEY_F2          = 0x3C,
    KEY_F3          = 0x3D,
    KEY_F4          = 0x3E,
    KEY_F5          = 0x3F,
    KEY_F6          = 0x40,
    KEY_F7          = 0x41,
    KEY_F8          = 0x42,
    KEY_F9          = 0x43,
    KEY_F10         = 0x44,
    KEY_F11         = 0x57, /* weird */
    KEY_F12         = 0x58,
    KEY_HOME        = 0x47,
    KEY_END         = 0x4F,
    KEY_INSERT      = 0x52,
    KEY_DEL         = 0x53,
    KEY_CAPS_LOCK   = 0x3A,
    KEY_LSHIFT      = 0x2A,
    KEY_RSHIFT      = 0x36,
    KEY_LCTRL       = 0x1D,
    KEY_RCTRL       = 0x1D,
    KEY_ALT         = 0x38,
    KEY_ALT_GR      = 0x38,
    KEY_TAB         = 0x0F,
    KEY_PRINT_SCREEN= 0x37,
    KEY_PG_UP       = 0x49,
    KEY_PG_DOWN     = 0x51,
    KEY_UP          = 0x48,
    KEY_DOWN        = 0x50,
    KEY_LEFT        = 0x4B,
    KEY_RIGHT       = 0x4D,
    KEY_FN          = 0x00,
    KEY_SUPER       = 0x5B,
    KEY_Q           = 0x10,
    KEY_W           = 0x11,
    KEY_E           = 0x12,
    KEY_R           = 0x13
    /* TODO */
};

/* Index for the keyboard mapping */
enum KEYBOARD_KEY_INDEX {
      KBI_INVALID, KBI_ESC
    , KBI_F1, KBI_F2, KBI_F3, KBI_F4, KBI_F5, KBI_F6
    , KBI_F7, KBI_F8, KBI_F9, KBI_F10, KBI_F11, KBI_F12
    , KBI_HOME, KBI_END, KBI_INSERT, KBI_DEL
    , KBI_CAPS_LOCK, KBI_LSHIFT, KBI_RSHIFT, KBI_LCTRL, KBI_RCTRL
    , KBI_ALT, KBI_LALT = KBI_ALT, KBI_ALT_GR, KBI_RALT = KBI_ALT_GR
    , KBI_PRINT_SCREEN, KBI_PG_UP, KBI_PG_DOWN
    , KBI_UP, KBI_DOWN, KBI_LEFT, KBI_RIGHT
    , KBI_FN, KBI_SUPER
};

typedef int (*key_handler_ft)(char scancode);

void keyboard_set_keymap(const char *);
uint8_t keyboard_get_scancode(void);
uint8_t keyboard_scancode_for(enum KEYBOARD_KEY_INDEX spec_key);
char keyboard_read(uint8_t *scancode);
void keyboard_loop(key_handler_ft handler);
void sys_reboot(void);

#endif // KEYBOARD_H
