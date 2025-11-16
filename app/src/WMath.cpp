#include "WMath.h"

#include <math.h>
#include <stdlib.h>

// Random number generation using libc's rand().
// Use arduino_ prefix to avoid conflict with POSIX random().
extern "C" long arduino_random(long max) {
  if (max <= 0) return 0;
  return static_cast<long>(rand() % static_cast<unsigned long>(max));
}

#ifdef __cplusplus
// C++ overload.
long arduino_random(long min, long max) {
  if (min >= max) return min;
  return static_cast<long>(rand() % static_cast<unsigned long>(max - min)) + min;
}
#else
// C version with different name.
long arduino_random_range(long min, long max) {
  if (min >= max) return min;
  return static_cast<long>(rand() % static_cast<unsigned long>(max - min)) + min;
}
#endif

extern "C" void randomSeed(unsigned long seed) {
  srand(static_cast<unsigned int>(seed));
}

extern "C" long map(long x, long in_min, long in_max, long out_min,
                    long out_max) {
  // Handle edge cases.
  if (in_max == in_min) {
    return (out_min + out_max) / 2;
  }

  // Standard mapping formula.
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

