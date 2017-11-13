/*
 * +---------------------------------------------------------------------+
 * | Copyright (C) Kovács Ákos - 2017                                    |
 * |                                                                     |
 * | Interrupt handling stuff                                            |
 * +---------------------------------------------------------------------+
*/

#include <basic.h>
#include <types.h>

struct __packed IdtPointer {
};

struct __packed Idt32Entry {
    uint16_t ie_offset_0_15;    // Offset bits [0..15]
    uint16_t ie_selector;
    uint8_t  ie_reserved;       // Always zero
    uint8_t  ie_type_attribute;
    uint16_t ie_offset_16_31;   // Offset bits [16..31]
};