/* 
 * Byte order conversion support functions, imported from Linux v3.10. 
 * Types are modified to conform to the integer typedefs used within the project
*/

#ifndef _LINUX_BYTEORDER_GENERIC_H
#define _LINUX_BYTEORDER_GENERIC_H

#include <types.h>
#include "swab.h"

/*
 * linux/byteorder_generic.h
 * Generic Byte-reordering support
 *
 * The "... p" macros, like le64_to_cpup, can be used with pointers
 * to unaligned data, but there will be a performance penalty on 
 * some architectures.  Use get_unaligned for unaligned data.
 *
 * Francois-Rene Rideau <fare@tunes.org> 19970707
 *    gathered all the good ideas from all asm-foo/byteorder.h into one file,
 *    cleaned them up.
 *    I hope it is compliant with non-GCC compilers.
 *    I decided to put __BYTEORDER_HAS_U64__ in byteorder.h,
 *    because I wasn't sure it would be ok to put it in types.h
 *    Upgraded it to 2.1.43
 * Francois-Rene Rideau <fare@tunes.org> 19971012
 *    Upgraded it to 2.1.57
 *    to please Linus T., replaced huge #ifdef's between little/big endian
 *    by nestedly #include'd files.
 * Francois-Rene Rideau <fare@tunes.org> 19971205
 *    Made it to 2.1.71; now a facelift:
 *    Put files under include/linux/byteorder/
 *    Split swab from generic support.
 *
 * TODO:
 *   = Regular kernel maintainers could also replace all these manual
 *    byteswap macros that remain, disseminated among drivers,
 *    after some grep or the sources...
 *   = Linus might want to rename all these macros and files to fit his taste,
 *    to fit his personal naming scheme.
 *   = it seems that a few drivers would also appreciate
 *    nybble swapping support...
 *   = every architecture could add their byteswap macro in asm/byteorder.h
 *    see how some architectures already do (i386, alpha, ppc, etc)
 *   = cpu_to_beXX and beXX_to_cpu might some day need to be well
 *    distinguished throughout the kernel. This is not the case currently,
 *    since little endian, big endian, and pdp endian machines needn't it.
 *    But this might be the case for, say, a port of Linux to 20/21 bit
 *    architectures (and F21 Linux addict around?).
 */

/*
 * The following macros are to be defined by <asm/byteorder.h>:
 *
 * Conversion of long and short int between network and host format
 *	ntohl(uint32_t x)
 *	ntohs(uint16_t x)
 *	htonl(uint32_t x)
 *	htons(uint16_t x)
 * It seems that some programs (which? where? or perhaps a standard? POSIX?)
 * might like the above to be functions, not macros (why?).
 * if that's true, then detect them, and take measures.
 * Anyway, the measure is: define only ___ntohl as a macro instead,
 * and in a separate file, have
 * unsigned long inline ntohl(x){return ___ntohl(x);}
 *
 * The same for constant arguments
 *	__constant_ntohl(uint32_t x)
 *	__constant_ntohs(uint16_t x)
 *	__constant_htonl(uint32_t x)
 *	__constant_htons(uint16_t x)
 *
 * Conversion of XX-bit integers (16- 32- or 64-)
 * between native CPU format and little/big endian format
 * 64-bit stuff only defined for proper architectures
 *	cpu_to_[bl]eXX(__uXX x)
 *	[bl]eXX_to_cpu(__uXX x)
 *
 * The same, but takes a pointer to the value to convert
 *	cpu_to_[bl]eXXp(__uXX x)
 *	[bl]eXX_to_cpup(__uXX x)
 *
 * The same, but change in situ
 *	cpu_to_[bl]eXXs(__uXX x)
 *	[bl]eXX_to_cpus(__uXX x)
 *
 * See asm-foo/byteorder.h for examples of how to provide
 * architecture-optimized versions
 *
 */
#define __constant_htonl(x) ((__force int32_be_t)___constant_swab32((x)))
#define __constant_ntohl(x) ___constant_swab32((__force int32_be_t)(x))
#define __constant_htons(x) ((__force int16_be_t)___constant_swab16((x)))
#define __constant_ntohs(x) ___constant_swab16((__force int16_be_t)(x))
#define __constant_cpu_to_le64(x) ((__force int64_le_t)(uint64_t)(x))
#define __constant_le64_to_cpu(x) ((__force uint64_t)(int64_le_t)(x))
#define __constant_cpu_to_le32(x) ((__force int32_le_t)(uint32_t)(x))
#define __constant_le32_to_cpu(x) ((__force uint32_t)(int32_le_t)(x))
#define __constant_cpu_to_le16(x) ((__force int16_le_t)(uint16_t)(x))
#define __constant_le16_to_cpu(x) ((__force uint16_t)(int16_le_t)(x))
#define __constant_cpu_to_be64(x) ((__force int64_be_t)___constant_swab64((x)))
#define __constant_be64_to_cpu(x) ___constant_swab64((__force uint64_t)(int64_be_t)(x))
#define __constant_cpu_to_be32(x) ((__force int32_be_t)___constant_swab32((x)))
#define __constant_be32_to_cpu(x) ___constant_swab32((__force uint32_t)(int32_be_t)(x))
#define __constant_cpu_to_be16(x) ((__force int16_be_t)___constant_swab16((x)))
#define __constant_be16_to_cpu(x) ___constant_swab16((__force uint16_t)(int16_be_t)(x))
#define __cpu_to_le64(x) ((__force int64_le_t)(uint64_t)(x))
#define le64_to_cpu(x) ((__force uint64_t)(int64_le_t)(x))
#define __cpu_to_le32(x) ((__force int32_le_t)(uint32_t)(x))
#define le32_to_cpu(x) ((__force uint32_t)(int32_le_t)(x))
#define __cpu_to_le16(x) ((__force int16_le_t)(uint16_t)(x))
#define le16_to_cpu(x) ((__force uint16_t)(int16_le_t)(x))
#define __cpu_to_be64(x) ((__force int64_be_t)__swab64((x)))
#define be64_to_cpu(x) __swab64((__force uint64_t)(int64_be_t)(x))
#define __cpu_to_be32(x) ((__force int32_be_t)__swab32((x)))
#define be32_to_cpu(x) __swab32((__force uint32_t)(int32_be_t)(x))
#define __cpu_to_be16(x) ((__force int16_be_t)__swab16((x)))
#define be16_to_cpu(x) __swab16((__force uint16_t)(int16_be_t)(x))

static inline uint64_le_t __cpu_to_le64p(const uint64_t *p)
{
    return (__force int64_le_t)*p;
}
static inline uint64_t __le64_to_cpup(const uint64_le_t *p)
{
    return (__force uint64_t)*p;
}
static inline uint32_le_t __cpu_to_le32p(const uint32_t *p)
{
    return (__force uint32_le_t)*p;
}
static inline uint32_t le32_to_cpup(const uint32_le_t *p)
{
    return (__force uint32_t)*p;
}
static inline uint16_le_t __cpu_to_le16p(const uint16_t *p)
{
    return (__force uint16_le_t)*p;
}
static inline uint16_t le16_to_cpup(const uint16_le_t *p)
{
    return (__force uint16_t)*p;
}
static inline uint64_be_t __cpu_to_be64p(const uint64_t *p)
{
    return (__force uint64_be_t)__swab64p(p);
}
static inline uint64_t be64_to_cpup(const uint64_be_t *p)
{
    return __swab64p((uint64_t *)p);
}
static inline uint32_be_t __cpu_to_be32p(const uint32_t *p)
{
    return (__force uint32_be_t)__swab32p(p);
}
static inline uint32_t be32_to_cpup(const uint32_be_t *p)
{
    return __swab32p((uint32_t *)p);
}
static inline uint16_be_t __cpu_to_be16p(const uint16_t *p)
{
    return (__force uint16_be_t)__swab16p(p);
}
static inline uint16_t be16_to_cpup(const uint16_be_t *p)
{
    return __swab16p((uint16_t *)p);
}
#define __cpu_to_le64s(x) do { (void)(x); } while (0)
#define le64_to_cpus(x) do { (void)(x); } while (0)
#define __cpu_to_le32s(x) do { (void)(x); } while (0)
#define le32_to_cpus(x) do { (void)(x); } while (0)
#define __cpu_to_le16s(x) do { (void)(x); } while (0)
#define le16_to_cpus(x) do { (void)(x); } while (0)
#define __cpu_to_be64s(x) __swab64s((x))
#define be64_to_cpus(x) __swab64s((x))
#define __cpu_to_be32s(x) __swab32s((x))
#define be32_to_cpus(x) __swab32s((x))
#define __cpu_to_be16s(x) __swab16s((x))
#define be16_to_cpus(x) __swab16s((x))

/* from include/linux/unaligned/be_byteshift.h */
static inline uint16_t __get_unaligned_be16(const uint8_t *p)
{
	return p[0] << 8 | p[1];
}

static inline uint32_t __get_unaligned_be32(const uint8_t *p)
{
	return p[0] << 24 | p[1] << 16 | p[2] << 8 | p[3];
}

static inline uint64_t __get_unaligned_be64(const uint8_t *p)
{
	return (uint64_t)__get_unaligned_be32(p) << 32 |
	       __get_unaligned_be32(p + 4);
}

static inline void __put_unaligned_be16(uint16_t val, uint8_t *p)
{
	*p++ = val >> 8;
	*p++ = val;
}

static inline void __put_unaligned_be32(uint32_t val, uint8_t *p)
{
	__put_unaligned_be16(val >> 16, p);
	__put_unaligned_be16(val, p + 2);
}

static inline void __put_unaligned_be64(uint64_t val, uint8_t *p)
{
	__put_unaligned_be32(val >> 32, p);
	__put_unaligned_be32(val, p + 4);
}

static inline uint16_t get_unaligned_be16(const void *p)
{
	return __get_unaligned_be16((const uint8_t *)p);
}

static inline uint32_t get_unaligned_be32(const void *p)
{
	return __get_unaligned_be32((const uint8_t *)p);
}

static inline uint64_t get_unaligned_be64(const void *p)
{
	return __get_unaligned_be64((const uint8_t *)p);
}

static inline void put_unaligned_be16(uint16_t val, void *p)
{
	__put_unaligned_be16(val, p);
}

static inline void put_unaligned_be32(uint32_t val, void *p)
{
	__put_unaligned_be32(val, p);
}

static inline void put_unaligned_be64(uint64_t val, void *p)
{
	__put_unaligned_be64(val, p);
}

/* from include/linux/unaligned/le_byteshift.h */
static inline uint16_t __get_unaligned_le16(const uint8_t *p)
{
	return p[0] | p[1] << 8;
}

static inline uint32_t __get_unaligned_le32(const uint8_t *p)
{
	return p[0] | p[1] << 8 | p[2] << 16 | p[3] << 24;
}

static inline uint64_t __get_unaligned_le64(const uint8_t *p)
{
	return (uint64_t)__get_unaligned_le32(p + 4) << 32 |
	       __get_unaligned_le32(p);
}

static inline void __put_unaligned_le16(uint16_t val, uint8_t *p)
{
	*p++ = val;
	*p++ = val >> 8;
}

static inline void __put_unaligned_le32(uint32_t val, uint8_t *p)
{
	__put_unaligned_le16(val >> 16, p + 2);
	__put_unaligned_le16(val, p);
}

static inline void __put_unaligned_le64(uint64_t val, uint8_t *p)
{
	__put_unaligned_le32(val >> 32, p + 4);
	__put_unaligned_le32(val, p);
}

static inline uint16_t get_unaligned_le16(const void *p)
{
	return __get_unaligned_le16((const uint8_t *)p);
}

static inline uint32_t get_unaligned_le32(const void *p)
{
	return __get_unaligned_le32((const uint8_t *)p);
}

static inline uint64_t get_unaligned_le64(const void *p)
{
	return __get_unaligned_le64((const uint8_t *)p);
}

static inline void put_unaligned_le16(uint16_t val, void *p)
{
	__put_unaligned_le16(val, p);
}

static inline void put_unaligned_le32(uint32_t val, void *p)
{
	__put_unaligned_le32(val, p);
}

static inline void put_unaligned_le64(uint64_t val, void *p)
{
	__put_unaligned_le64(val, p);
}

#endif /* _LINUX_BYTEORDER_GENERIC_H */
