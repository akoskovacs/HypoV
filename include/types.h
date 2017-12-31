/*
 * +------------------------------------------------------------+
 * | Copyright (C) Ákos Kovács - 2017                           |
 * |                                                            |
 * | Typedefs for 64 bit platform specific data sizes           |
 * +------------------------------------------------------------+
*/
#ifndef TYPES_H
#define TYPES_H

/* Only valid for 64 bit GCCs */
#ifdef __HVISOR__

typedef unsigned long size_t;
typedef unsigned long long off_t;

typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

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