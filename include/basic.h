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

#define typeof __typeof__
//#define __CHECKER__

/* Helper macros from Linux v4.5 include/linux/compiler.h */
#define ___PASTE(a,b) a##b
#define __PASTE(a,b) ___PASTE(a,b)

#ifndef likely
# define likely(x)	(__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 1))
#endif
#ifndef unlikely
# define unlikely(x)	(__builtin_constant_p(x) ? !!(x) : __branch_check__(x, 0))
#endif

#ifdef __CHECKER__
# define __force	__attribute__((force))
#else //__CHECKER__
# define __force
#endif // __CHECKER__

/* Not-quite-unique ID. */
#ifndef __UNIQUE_ID
# define __UNIQUE_ID(prefix) __PASTE(__PASTE(__UNIQUE_ID_, prefix), __LINE__)
#endif

/* Helper macros, imported from Linux v3.10 include/linux/kernel.h */

/*
 * min()/max()/clamp() macros that also do
 * strict type-checking.. See the
 * "unnecessary" pointer comparison.
 */
#define __min(t1, t2, min1, min2, x, y) ({		\
    t1 min1 = (x);					\
    t2 min2 = (y);					\
    (void) (&min1 == &min2);			\
    min1 < min2 ? min1 : min2; })

/**
 * min - return minimum of two values of the same or compatible types
 * @x: first value
 * @y: second value
 */
#define min(x, y)					\
    __min(typeof(x), typeof(y),			\
          __UNIQUE_ID(min1_), __UNIQUE_ID(min2_),	\
          x, y)

#define __max(t1, t2, max1, max2, x, y) ({		\
    t1 max1 = (x);					\
    t2 max2 = (y);					\
    (void) (&max1 == &max2);			\
    max1 > max2 ? max1 : max2; })

/**
 * max - return maximum of two values of the same or compatible types
 * @x: first value
 * @y: second value
 */
#define max(x, y)					\
    __max(typeof(x), typeof(y),			\
          __UNIQUE_ID(max1_), __UNIQUE_ID(max2_),	\
          x, y)

/**
 * min3 - return minimum of three values
 * @x: first value
 * @y: second value
 * @z: third value
 */
#define min3(x, y, z) min((typeof(x))min(x, y), z)

/**
 * max3 - return maximum of three values
 * @x: first value
 * @y: second value
 * @z: third value
 */
#define max3(x, y, z) max((typeof(x))max(x, y), z)

/**
 * min_not_zero - return the minimum that is _not_ zero, unless both are zero
 * @x: value1
 * @y: value2
 */
#define min_not_zero(x, y) ({			\
    typeof(x) __x = (x);			\
    typeof(y) __y = (y);			\
    __x == 0 ? __y : ((__y == 0) ? __x : min(__x, __y)); })

/**
 * clamp - return a value clamped to a given range with strict typechecking
 * @val: current value
 * @lo: lowest allowable value
 * @hi: highest allowable value
 *
 * This macro does strict typechecking of @lo/@hi to make sure they are of the
 * same type as @val.  See the unnecessary pointer comparisons.
 */
#define clamp(val, lo, hi) min((typeof(val))max(val, lo), hi)

/*
 * ..and if you can't take the strict
 * types, you can specify one yourself.
 *
 * Or not use min/max/clamp at all, of course.
 */

/**
 * min_t - return minimum of two values, using the specified type
 * @type: data type to use
 * @x: first value
 * @y: second value
 */
#define min_t(type, x, y)				\
    __min(type, type,				\
          __UNIQUE_ID(min1_), __UNIQUE_ID(min2_),	\
          x, y)

/**
 * max_t - return maximum of two values, using the specified type
 * @type: data type to use
 * @x: first value
 * @y: second value
 */
#define max_t(type, x, y)				\
    __max(type, type,				\
          __UNIQUE_ID(min1_), __UNIQUE_ID(min2_),	\
          x, y)

/**
 * clamp_t - return a value clamped to a given range using a given type
 * @type: the type of variable to use
 * @val: current value
 * @lo: minimum allowable value
 * @hi: maximum allowable value
 *
 * This macro does no typechecking and uses temporary variables of type
 * @type to make all the comparisons.
 */
#define clamp_t(type, val, lo, hi) min_t(type, max_t(type, val, lo), hi)

/**
 * clamp_val - return a value clamped to a given range using val's type
 * @val: current value
 * @lo: minimum allowable value
 * @hi: maximum allowable value
 *
 * This macro does no typechecking and uses temporary variables of whatever
 * type the input argument @val is.  This is useful when @val is an unsigned
 * type and @lo and @hi are literals that will otherwise be assigned a signed
 * integer type.
 */
#define clamp_val(val, lo, hi) clamp_t(typeof(val), val, lo, hi)


/**
 * swap - swap values of @a and @b
 * @a: first value
 * @b: second value
 */
#define swap(a, b) \
    do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:	the pointer to the member.
 * @type:	the type of the container struct this is embedded in.
 * @member:	the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({				\
    void *__mptr = (void *)(ptr);					\
    BUILD_BUG_ON_MSG(!__same_type(*(ptr), ((type *)0)->member) &&	\
             !__same_type(*(ptr), void),			\
             "pointer type mismatch in container_of()");	\
    ((type *)(__mptr - offsetof(type, member))); })

/* End of Linux stuff */

#endif // BASIC_H
