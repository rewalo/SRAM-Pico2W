#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

// Math constants (also defined in common.h, but included here for completeness).
// Note: We don't include <stdlib.h> here to avoid conflict with POSIX random().
// The implementation includes it where needed.
#ifndef PI
#define PI 3.14159265358979323846
#endif
#ifndef DEG_TO_RAD
#define DEG_TO_RAD 0.01745329251994329577
#endif
#ifndef RAD_TO_DEG
#define RAD_TO_DEG 57.2957795130823208768
#endif

// Math macros (also in common.h, but included here for completeness).
#ifndef constrain
#define constrain(x, l, h) ((x) < (l) ? (l) : ((x) > (h) ? (h) : (x)))
#endif
#ifndef radians
#define radians(deg) ((deg) * DEG_TO_RAD)
#endif
#ifndef degrees
#define degrees(rad) ((rad) * RAD_TO_DEG)
#endif
#ifndef sq
#define sq(x) ((x) * (x))
#endif

// Random number generation using libc's rand() and srand() functions.
// Note: We use arduino_ prefix internally to avoid conflict with POSIX random().
// Base function - C compatible (single argument, inside extern "C").
long arduino_random(long max);
#ifdef __cplusplus
// C++ will have overload declared outside extern "C".
#else
// C version - use different name since C doesn't support overloading.
long arduino_random_range(long min, long max);
#endif
void randomSeed(unsigned long seed);

// Maps a value from one range to another.
long map(long x, long in_min, long in_max, long out_min, long out_max);

#ifdef __cplusplus
}
// C++ overload - declared outside extern "C" for proper overloading.
long arduino_random(long min, long max);

// Arduino-compatible wrappers using macros to override POSIX random().
// Macros are processed before function declarations, so they take precedence.
// Note: Include WMath.h BEFORE <stdlib.h> for best compatibility.
#define random(...) _arduino_random_impl(__VA_ARGS__)
inline long _arduino_random_impl(long max) {
  return arduino_random(max);
}
inline long _arduino_random_impl(long min, long max) {
  return arduino_random(min, max);
}
#else
// Arduino-compatible macros for C.
#define random(max) arduino_random(max)
#define random_range(min, max) arduino_random_range(min, max)
#endif

