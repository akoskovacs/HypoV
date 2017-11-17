#include <string.h>
#include <system.h>
#include <drivers/input/pc_keyboard.h>

#define KBD_CMD_REBOOT 0xFE
#define KBD_PORT 0x60

/* TODO... */
#if KBD_HAVE_UNICODE
const char kbd_hu_keymap[] = "\0\000123456789öüó\b\t"
            /* 0x10 */ "qwertzuiopőú\n\0as"
            /* 0x20 */ "dfghjkléá0\0űyxcv"
            /* 0x30 */ "bnm,.-\0\0\0 ";
            /* TODO: í */
#else
const char kbd_hu_keymap[] = "\0\000123456789ouo\b\t"
            /* 0x10 */ "qwertzuiopou\n\0as"
            /* 0x20 */ "dfghjklea0\0uyxcv"
            /* 0x30 */ "bnm,.-\0\0\0 ";

#endif // KBD_HAVE_UNICODE
#define KBD_HU_KEYMAP_SIZE sizeof(kbd_hu_keymap)/sizeof(char)
/* XXX: Must be in the order with enum KEYBOARD_KEY_INDEX */
const char kbd_hu_spec_keys[] = {
    /* invalid key      */ 0x00,
    /* KBI_ESC          */ 0x01,
    /* KBI_F1           */ 0x3B,
    /* KBI_F2           */ 0x3C,
    /* KBI_F3           */ 0x3D,
    /* KBI_F4           */ 0x3E,
    /* KBI_F5           */ 0x3F,
    /* KBI_F6           */ 0x40,
    /* KBI_F7           */ 0x41,
    /* KBI_F8           */ 0x42,
    /* KBI_F9           */ 0x43,
    /* KBI_F10          */ 0x44,
    /* KBI_F11          */ 0x57, /* weird */
    /* KBI_F12          */ 0x58,
    /* KBI_HOME         */ 0x47,
    /* KBI_END          */ 0x4F,
    /* KBI_INSERT       */ 0x52,
    /* KBI_DEL          */ 0x53,
    /* KBI_CAPS_LOCK    */ 0x3A,
    /* KBI_LSHIFT       */ 0x2A,
    /* KBI_RSHIFT       */ 0x36,
    /* KBI_LCTRL        */ 0x1D,
    /* KBI_RCTRL        */ 0x1D,
    /* KBI_ALT          */ 0x38,
    /* KBI_ALT_GR       */ 0x38,
    /* KBI_PRINT_SCREEN */ 0x37,
    /* KBI_PG_UP        */ 0x49,
    /* KBI_PG_DOWN      */ 0x51,
    /* KBI_UP           */ 0x48,
    /* KBI_DOWN         */ 0x50,
    /* KBI_LEFT         */ 0x4B,
    /* KBI_RIGHT        */ 0x4D,
    /* KBI_FN           */ 0x00,
    /* KBI_SUPER        */ 0x5B,
};
#define SPEC_KEY_FIRST 0
#define SPEC_KEY_LAST (sizeof(kbd_hu_spec_keys)/sizeof(uint8_t)-1)

static const char *kbd_keymap = kbd_hu_keymap;
static const uint8_t *kbd_spec_keys = kbd_hu_spec_keys;
static size_t kbd_keymap_size = KBD_HU_KEYMAP_SIZE;
static bool kbd_is_shift_on = false;
static bool kbd_is_caps_lock_on = false;

void keyboard_set_keymap(const char *map_name)
{
    if (map_name[0] == 'H' && map_name[1] == 'U' 
            && map_name[2] == '\0') {
        kbd_keymap      = kbd_hu_keymap;
        kbd_spec_keys   = kbd_hu_spec_keys;
        kbd_keymap_size = KBD_HU_KEYMAP_SIZE;
    }
}

uint8_t keyboard_scancode_for(enum KEYBOARD_KEY_INDEX k)
{
    if (k >= SPEC_KEY_FIRST && k <= SPEC_KEY_LAST) {
        return kbd_spec_keys[k];
    }
    return KBI_INVALID;
}

uint8_t keyboard_get_scancode(void)
{
    uint8_t c = 0;
    do {
        if (inb(KBD_PORT) != c) {
            c = inb(KBD_PORT);
            if (c > 0) {
                return c;
            }
        }
    } while (1);
}

char keyboard_read(uint8_t *scancode)
{
    uint8_t sc = keyboard_get_scancode();
    if (scancode != NULL) {
        *scancode = sc;
    }
    if (sc < kbd_keymap_size) {
        return kbd_keymap[sc];
    }
    return '\0';
}

/* On i386 the reset is handled by the keyboard controller */
void sys_reboot(void)
{
    char good = 0x02;
    while (good & 0x02) {
        good = inb(0x64);
        inb(KBD_PORT);
    }
    outb(0x64, KBD_CMD_REBOOT);

    /* Halt anyway */
    forever {
        halt();
    }
}

void __noreturn keyboard_loop(key_handler_ft key_handler)
{
    bool is_prev_ctrl = false;
    bool is_prev_alt = false;
    bool is_prev_ctrl_alt = false;
    char scancode;
    int i = 0;
    forever {
        char ch = keyboard_read(&scancode);
        if (ch == KEY_RCTRL || ch == KEY_LCTRL) {
            is_prev_ctrl = true;
            is_prev_alt = false;
            is_prev_ctrl_alt = false;
        } else if (ch == KEY_ALT || ch == KEY_ALT_GR) {
            is_prev_alt = true;
            if (is_prev_ctrl) {
                is_prev_ctrl_alt = true;
            }
        } else if (ch == KEY_DEL) {
            if (is_prev_ctrl_alt) {
                sys_reboot();
            }
        } else {
            is_prev_ctrl = false;
            is_prev_alt = false;
            is_prev_ctrl_alt = false;

            if (key_handler) {
                key_handler(scancode);
            }
        }
    }
}