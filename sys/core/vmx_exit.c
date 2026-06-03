/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2024                           |
 * |                                                            |
 * | VM exit handler (stub — Phase 4)                           |
 * +------------------------------------------------------------+
*/
#include <vmx.h>
#include <print.h>

extern struct CharacterDisplay debug_serial;

void vmx_exit_handler(struct GuestRegs *regs)
{
    uint32_t reason = (uint32_t)vmx_read(VMCS_VM_EXIT_REASON) & 0xFFFF;
    (void)regs;
    /* Phase 4 will dispatch on reason */
    hv_printf(&debug_serial, "VM exit: reason %d\n", reason);
}
