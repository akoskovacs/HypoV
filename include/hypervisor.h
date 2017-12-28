/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Hypervisor system functions                                |
 * +------------------------------------------------------------+
*/
#ifndef HYPERVISOR_H
#define HYPERVISOR_H

struct MultiBootInfo;

void hv_entry(struct MultiBootInfo *);

#endif // HYPERVISOR_H
