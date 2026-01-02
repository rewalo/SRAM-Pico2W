#pragma once
#include <stddef.h>
#include <stdint.h>
#include <Arduino.h>  // For Serial

// SRAM region considered safe for passing pointers between app and kernel.
//
// IMPORTANT:
// In this architecture the app's `setup()`/`loop()` run inside a FreeRTOS task
// created by the kernel. That means many "app" temporary buffers (stack locals)
// can live on the *kernel task stack* allocated by FreeRTOS, which may be below
// 0x20030000. If we validate only the SRAM app image window, we incorrectly
// reject perfectly valid pointers (e.g. Serial print buffers, small arrays like
// `uint8_t mac[6]`).
//
// So we validate against the full RP2350 SRAM span used by this project.
#define kSramBaseAddr (0x20000000u)
#define kSramSize (0x000A0000u)  // 640KB -> end at 0x200A0000
#define kSramEndAddr (kSramBaseAddr + kSramSize)

namespace syscall_validation {

// Checks if pointer is within app SRAM region
// Also validates buffer doesn't overflow if len > 0
inline bool isValidAppPointer(const void* ptr, size_t len) {
  if (ptr == nullptr) return false;
  uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);
  if (addr < kSramBaseAddr || addr >= kSramEndAddr) return false;
  if (len > 0 && addr > kSramEndAddr - len) return false;
  return true;
}

// String validation - ensures at least 1 byte can be safely read
inline bool isValidAppString(const char* str) {
  bool valid = isValidAppPointer(str, 1);
  if (!valid && str != nullptr) {
    // Debug: print the actual address that failed
    uintptr_t addr = reinterpret_cast<uintptr_t>(str);
    Serial.print("[Kernel] Pointer validation failed: 0x");
    Serial.print(addr, HEX);
    Serial.print(" (SRAM: 0x");
    Serial.print(kSramBaseAddr, HEX);
    Serial.print("-0x");
    Serial.print(kSramEndAddr, HEX);
    Serial.println(")");
  }
  return valid;
}

}