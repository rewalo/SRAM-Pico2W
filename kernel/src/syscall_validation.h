#pragma once
#include <stddef.h>
#include <stdint.h>
#include <Arduino.h>  // For Serial

// SRAM region where app code runs
#define kSramBaseAddr (0x20030000u)
#define kSramSize (0x00070000u)
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