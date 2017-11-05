/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor initialization code                             |
 * +------------------------------------------------------------+
*/
#include <basic.h>
#include <boot/multiboot.h>
#include <types.h>
#include <hypervisor.h>

volatile uint16_t *videomem = (uint16_t *)0xb8000;
#define CONSOLE_WIDTH  80
#define CONSOLE_HEIGHT 25

void __noreturn 
hv_entry(struct MultiBootInfo *mbi, unsigned int magic)
{
    int i;
    const char hw[] = "HypoV - Copyright (C) Akos Kovacs";
    for (i = 0; i < CONSOLE_HEIGHT*CONSOLE_WIDTH; i++) {
        videomem[i] = ((0x47) << 8);
    }

    for (i = 0; i < sizeof(hw); i++) {
        videomem[i] = (0x47 << 8) | hw[i];
    }

    while (1) 
        ;
}
