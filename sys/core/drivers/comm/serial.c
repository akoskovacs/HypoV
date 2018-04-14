/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2018                           |
 * |                                                            |
 * | Serial driver, mostly used for debugging                   |
 * +------------------------------------------------------------+
*/
#include <basic.h>
#include <system.h>
#include <char_display.h>

#define COM1_PORT 0x3F8

bool serial_is_tx_empty(void)
{
    return inb(COM1_PORT + 5) & 0x20;
}

int serial_putc(struct CharacterDisplay *display, char ch)
{
    /* Only send bytes, if the transmit FIFO is empty */
    while (serial_is_tx_empty() == 0)
        ;

    outb(COM1_PORT, ch);
    return 0;
}

int serial_putc_xy(struct CharacterDisplay *display, int x, int y, char ch)
{
    (void)x;
    (void)y;
    return serial_putc(display, ch);
}

int serial_setup(struct CharacterDisplay *display)
{
    /* Disable all interrupts */
    outb(COM1_PORT + 1, 0x0);
    /* Enable DLAB (bad rate divisor) */
    outb(COM1_PORT + 3, 0x80);
    /* Division for 9600 Baud, low and high byte */
    outb(COM1_PORT + 0, 0x0C);
    outb(COM1_PORT + 1, 0x0);
    /* 8 bits, no parity, one stop bit (standard configuration) */
    outb(COM1_PORT + 3, 0x03);
    /* Enable FIFO, clear them, with 14-byte threshold */
    outb(COM1_PORT + 2, 0xC7); 
    /* IRQs enabled, RTS/DSR set */
    //outb(COM1_PORT + 4, 0x0B);
    return 0;
}

int hv_serial_init(struct CharacterDisplay *display)
{
    display->disp_name       = "serial";
    display->disp_setup      = serial_setup;
    display->disp_clear      = NULL;
    display->disp_get_max_xy = NULL;
    display->disp_get_xy     = NULL;
    display->disp_putc       = serial_putc;
    display->disp_putc_xy    = serial_putc_xy;
    return 0;
}