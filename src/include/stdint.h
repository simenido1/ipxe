#ifndef _STDINT_H
#define _STDINT_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/*
 * This is a standard predefined macro on all gcc's I've seen. It's
 * important that we define size_t in the same way as the compiler,
 * because that's what it's expecting when it checks %zd/%zx printf
 * format specifiers.
 */
#ifndef __SIZE_TYPE__
#define __SIZE_TYPE__ unsigned long /* safe choice on most systems */
#endif

#include <bits/stdint.h>

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef int64_t int64;
typedef uint64_t uint64;

typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

// #  define __INT64_C(c)	c ## L
// #  define __UINT64_C(c)	c ## UL

/* Limits of integral types.  */

/* Minimum of signed integral types.  */
# define INT8_MIN		(-128)
# define INT16_MIN		(-32767-1)
# define INT32_MIN		(-2147483647-1)
# define INT64_MIN		(-__INT64_C(9223372036854775807)-1)
/* Maximum of signed integral types.  */
# define INT8_MAX		(127)
# define INT16_MAX		(32767)
# define INT32_MAX		(2147483647)
# define INT64_MAX		(__INT64_C(9223372036854775807))

/* Maximum of unsigned integral types.  */
# define UINT8_MAX		(255)
# define UINT16_MAX		(65535)
# define UINT32_MAX		(4294967295U)
# define UINT64_MAX		(__UINT64_C(18446744073709551615))

#define UINT64_C(val) val##ULL
#define INT64_C(val) val##LL




#endif /* _STDINT_H */
