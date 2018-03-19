/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Basic system functions (port input/output/etc)             |
 * +------------------------------------------------------------+
*/
#ifndef SYSTEM_H
#define SYSTEM_H

#include <types.h>
#include <basic.h>

enum CPUID_REGISTERS {
    CPUID_REG_EAX = 0,
    CPUID_REG_EBX,
    CPUID_REG_ECX,
    CPUID_REG_EDX
};

/* 
 * Implemented by the keyboard driver, because on PCs the keyboard
 * controller is routed to the reset line
*/
void sys_reboot(void);
void sys_chainload(void) __noreturn;

int cpuid_get_branding(char branding[49]);
int cpuid_get_vendor(char vendor[13]);

static inline
void bochs_breakpoint(void)
{
    __asm__ __volatile__("xchg %bx, %bx");
}

/* These helper functions are mostly based on 
   OSDev's inline assembly functions */

static inline
void outb(uint16_t port, uint8_t value)
{
    __asm__ __volatile__("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline
void outw(uint16_t port, uint16_t value)
{
    __asm__ __volatile__("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline
int cpuid(int32_t leaf, int32_t output[4])
{
    __asm__ __volatile__("cpuid"
            : "=a"(*output), "=b"(*(output+1)), "=c"(*(output+2)), "=d"(*(output+3))
            : "a"(leaf)
    );

    return output[0];
}

static inline
uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

static inline
uint16_t inw(uint16_t port)
{
    uint16_t value;
    __asm__ __volatile__("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

#if 0
static inline bool is_irq_on()
{
    int f;
    __asm__ __volatile__("pushf\n\tpopl %0"
    : "=g"(f));
    return !!(f & (1<<9)); /* Must be a bool (8 bit size) */
}
#endif

static inline
void rdtsc(uint32_t *upper, uint32_t *lower)
{
    __asm__ __volatile__("rdtsc" : "=a"(*lower), "=d"(*upper));
}

static inline
uint32_t cr0_read(void)
{
    uint32_t value;
    __asm__ __volatile__("movl %%cr0, %0" : "=r"(value));
    return value;
}

static inline
void cr0_write(uint32_t value)
{
    __asm__ __volatile("movl %0, %%cr0" : /* No output */
                                        : "r"(value));
}

static inline
uint32_t cr1_read(void)
{
    uint32_t value;
    __asm__ __volatile__("movl %%cr1, %0" : "=r"(value));
    return value;
}

static inline
uint32_t cr2_read(void)
{
    uint32_t value;
    __asm__ __volatile__("movl %%cr2, %0" : "=r"(value));
    return value;
}

static inline
uint32_t cr3_read(void)
{
    uint32_t value;
    __asm__ __volatile__("movl %%cr3, %0" : "=r"(value));
    return value;
}

static inline
void cr3_write(uint32_t value)
{
    __asm__ __volatile__("movl %0, %%cr3" : /* No output */
                                        : "r"(value));
}

static inline
uint32_t cr4_read(void)
{
    uint32_t value;
    __asm__ __volatile__("movl %%cr4, %0" : "=r"(value));
    return value;
}

static inline
void cr4_write(uint32_t value)
{
    __asm__ __volatile__("movl %0, %%cr4" : /* No output */
                                        : "r"(value));
}

static inline
uint64_t msr_read(uint32_t msr)
{
    uint32_t high;
    uint32_t low;
    __asm__ __volatile__("rdmsr" : "=d"(high), "=a"(low)
                                 : "c"(msr));
    return ((uint64_t)high << 32) | low;
}

static inline
void msr_write(uint32_t msr, uint64_t value)
{
    uint32_t high = value >> 32;
    uint32_t low = value & 0xFFFFFFFF;
    __asm__ __volatile__("wrmsr" : /* No output */
                                 : "c"(msr), "d"(high), "a"(low));
}

static inline
{
    __asm__ __volatile__("sti");
}

static inline
void int_disable(void)
{
    __asm__ __volatile__("cli");
}
/* Invalidate the Translation Lookaside Buffer in uniprocessor mode */
static inline
void tlb_flush(unsigned long addr)
{
    __asm__ __volatile__("invlpg (%0)" ::"r" (addr) : "memory");
}

static inline
void halt(void)
{
    __asm__ __volatile__("hlt");
}

#endif // SYSTEM_H
