/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Typedefs for 64 bit platform specific data sizes           |
 * +------------------------------------------------------------+
*/
#ifndef TYPES_H
#define TYPES_H

#define USHORT_MAX	((uint16_t)(~0U))
#define SHORT_MAX	((int16_t)(USHRT_MAX>>1))
#define SHORT_MIN	((sint16_t)(-SHRT_MAX - 1))
#define INT_MAX		((int)(~0U>>1))
#define INT_MIN		(-INT_MAX - 1)
#define UINT_MAX	(~0U)
#define LONG_MAX	((long)(~0UL>>1))
#define LONG_MIN	(-LONG_MAX - 1)
#define ULONG_MAX	(~0UL)
#define LLONG_MAX	((long long)(~0ULL>>1))
#define LLONG_MIN	(-LLONG_MAX - 1)
#define ULLONG_MAX	(~0ULL)
#define SIZE_MAX	(~(size_t)0)

#define UINT8_MAX		((uint8_t)~0U)
#define INT8_MAX		((int8_t)(UINT8_MAX>>1))
#define INT8_MIN		((int8_t)(-INT8_MAX - 1))
#define UINT16_MAX		((uint16_t)~0U)
#define INT16_MAX		((int16_t)(UINT16_MAX>>1))
#define INT16_MIN		((int16_t)(-INT16_MAX - 1))
#define UINT32_MAX		((uint32_t)~0U)
#define INT32_MAX		((int32_t)(UINT32_MAX>>1))
#define INT32_MIN		((int32_t)(-INT32_MAX - 1))
#define UINT64_MAX		((uint64_t)~0ULL)
#define INT64_MAX		((int64_t)(UINT64_MAX>>1))
#define INT64_MIN		((int64_t)(-INT64_MAX - 1))

#define __i386__ 1

#ifdef __HVISOR__
#if defined(__i386__)
    typedef unsigned long size_t;
    typedef unsigned long long off_t;

    typedef unsigned char uint8_t;
    typedef unsigned short int uint16_t;
    typedef unsigned int uint32_t;
    typedef unsigned long long uint64_t;

    /* Little endian version */
    typedef unsigned char uint8_le_t;
    typedef unsigned short int uint16_le_t;
    typedef unsigned int uint32_le_t;
    typedef unsigned long long uint64_le_t;

    /* Big endian version */
    typedef unsigned char uint8_be_t;
    typedef unsigned short int uint16_be_t;
    typedef unsigned int uint32_be_t;
    typedef unsigned long long uint64_be_t;

    typedef signed char int8_t;
    typedef signed short int int16_t;
    typedef signed int int32_t;
    typedef signed long long int64_t;

    /* Little endian */
    typedef signed char int8_le_t;
    typedef signed short int int16_le_t;
    typedef signed int int32_le_t;
    typedef signed long long int64_le_t;

    /* Big endian */
    typedef signed char int8_be_t;
    typedef signed short int int16_be_t;
    typedef signed int int32_be_t;
    typedef signed long long int64_be_t;
#elif defined()
    typedef unsigned long size_t;
    typedef unsigned long long off_t;

    typedef unsigned char uint8_t;
    typedef unsigned short int uint16_t;
    typedef unsigned int uint32_t;
    typedef unsigned long uint64_t;

    /* Little endian */
    typedef unsigned char uint8_le_t;
    typedef unsigned short int uint16_le_t;
    typedef unsigned int uint32_le_t;
    typedef unsigned long uint64_le_t;

    /* Big endian */
    typedef unsigned char uint8_be_t;
    typedef unsigned short int uint16_be_t;
    typedef unsigned int uint32_be_t;
    typedef unsigned long uint64_be_t;

    typedef signed char int8_t;
    typedef signed short int int16_t;
    typedef signed int int32_t;
    typedef signed long int64_t;

    /* Little endian */
    typedef signed char int8_le_t;
    typedef signed short int int16_le_t;
    typedef signed int int32_le_t;
    typedef signed long int64_le_t;

    /* Big endian */
    typedef signed char int8_be_t;
    typedef signed short int int16_be_t;
    typedef signed int int32_be_t;
    typedef signed long int64_be_t;
#endif // __i386__

#ifndef __cplusplus
typedef enum { false, true } bool;
#endif // __cplusplus

/* Hypervisor (host) physical addres */
typedef uint64_t hpa_t;
typedef hpa_t    pa_t;
typedef unsigned long npa_t; /* Native physical address */

/* Hypervisor (host) virtual addres */
typedef uint64_t hva_t;
typedef hva_t    va_t;
typedef unsigned long nva_t; /* Native virtual address */

typedef uint64_t gpa_t;  /* Guest OS physical addres */
typedef uint64_t gva_t;  /* Guest OS physical addres */

typedef uint64_t pml4_t; /* Page Map Level 4             */
typedef uint64_t pml3_t; /* Page Directory Pointer Table */
typedef uint64_t pml2_t; /* Page Directory Table         */
typedef uint64_t pml1_t; /* Unused for 2MB paging        */

#endif // __HVISOR__

#endif // TYPES_H