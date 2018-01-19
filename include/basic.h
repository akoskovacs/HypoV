/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Basic macros for compiler support                          |
 * +------------------------------------------------------------+
*/
#ifndef BASIC_H
#define BASIC_H

#define NULL ((void *)0)

/*
#define PAGE_SIZE   4096
#define PAGE_SHIFT  12
#define PAGE_OFFSET 0xC0000000
*/

//#define PAGE_ALIGN(addr) (vaddr_t)(((uint32_t)addr) & (~0x0F))
//#define ROUNDUP(x,y) ((x + ((y)-1)) & ~((y)-1))

/* Simple bit operations, works on the Nth bit of the value */
#define SET_BIT(V, N) ((V) |= (1 << (N)))
#define CLEAR_BIT(V, N) ((V) &= ~(1 << (N)))
#define IS_BIT_SET(V, N) TEST_BIT(V, N)
#define IS_BIT_NOT_SET(V, N) (!TEST_BIT(V, N))
#define TEST_BIT(V, N) ((V) & (1 << (N)))

/* General GCC attributes */
#define __inline __attribute__((__inline__))
#define __unused __attribute__((unused))
#define __used __attribute__((used))
#define __packed __attribute__((packed))
#define __align(A) __attribute__((__aligned__(A)))
#define __weak __attribute__((weak))
#define __naked __attribute__((naked))
#define __noreturn __attribute__((noreturn))
#define __always_inline __attribute__((always_inline))

#define __aligned_8 __attribute__((__aligned__(8)))
#define __aligned_16 __attribute__((__aligned__(16)))

/* Section macros for initcalls */
/*
#define __init __section(".init.text")
#define __initdata __section(".init.data")
#define __exit __section(".exit.text")
#define __exitdata __section(".exit.data")
#define __section(name) __attribute((__section__(#name)))
typedef int (*initcall_t)(void);
*/

/* Setup section */
/*
#define __setup __section(".setup")
#define __setup_data __section(".setup.data")
*/

/* Macros for va_list */
#define va_start(v,l)	__builtin_va_start(v,l)
#define va_end(v)	__builtin_va_end(v)
#define va_arg(v,l)	__builtin_va_arg(v,l)
#define va_copy(d,s)	__builtin_va_copy(d,s)
typedef __builtin_va_list va_list;

#define forever while(1)

#endif // BASIC_H
