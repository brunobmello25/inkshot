#ifndef BASE_CORE_H

/*
 * NOTE:
 * BUILD_SLOW
 *      0 - No slow code allowed (production or testing performance)
 *      1 - Slow code is allowed (extra logging, expensive sanity checks, etc.)
 *  BUILD_INTERNAL
 *      0 - Build for public release (no engine source code available)
 *      1 - Useful stuff like debug functions, console, specific memory
 * allocation addresses, etc
 */

#include <stdint.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef float f32;
typedef double f64;
typedef i32 b32;

#define true 1
#define false 0

#define local_persist static
#define global_variable static
#define internal static

#define ARRAY_COUNT(array) (sizeof(array) / sizeof(array[0]))

#define KB(x) ((u64)(x) * (u64)1024)
#define MB(x) ((u64)(KB(x)) * (u64)1024)
#define GB(x) ((u64)MB(x) * (u64)1024)
#define TB(x) ((u64)GB(x) * (u64)1024)

#define CLAMP(value, min, max)                                                 \
  ((value < min) ? min : (value > max) ? max : value)

#if BUILD_SLOW
#define Assert(expression)                                                     \
  if (!(expression)) {                                                         \
    *(volatile int *)0 = 0;                                                    \
  }
#else
#define ASSERT(expression)
#endif

#define AlignPow2(x, b) (((x) + (b) - 1) & ~((b) - 1))

#define MemorySet(dst, value, size) memset((dst), (value), (size))
#define MemoryZero(dst, size) memset((dst), 0, (size))
#define MemoryCopy(dst, src, size) memcpy((dst), (src), (size))

#define BASE_CORE_H
#endif
