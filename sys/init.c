/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor 64 bit initialization code                      |
 * +------------------------------------------------------------+
*/
#include <boot/multiboot.h>
#include <basic.h>
#include <hypervisor.h>

void __noreturn 
hv_entry(struct MultiBootInfo *mbi, unsigned int magic)
{
    while (1) 
        ;
}
