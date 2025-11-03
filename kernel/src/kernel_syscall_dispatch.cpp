#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <Wire.h>

#include "generated/syscall_ids.h"
#include "syscall_invoke.h"
#include "syscall_validation.h"

//#define DEBUG_SYSCALLS

// Wrappers that check string pointers before passing to kernel Serial
// Prevents crashes from invalid app pointers
namespace syscall_safe_wrappers {
static size_t serialPrintSafe(const char* s) {
  if (!syscall_validation::isValidAppString(s)) {
    Serial.print("[Kernel] ERROR: Invalid pointer in Serial.print");
    return 0;
  }
  return Serial.print(s);
}

static size_t serialPrintlnSafe(const char* s) {
  if (!syscall_validation::isValidAppString(s)) {
    Serial.print("[Kernel] ERROR: Invalid pointer in Serial.println");
    Serial.println();
    return 0;
  }
  return Serial.println(s);
}

// SPI wrapper - beginTransaction with default settings
static void spiBeginTransaction() {
  SPI.beginTransaction(SPISettings());  // Default settings
}
}

// Include jump table handlers after namespace is defined (they reference syscall_safe_wrappers)
#include "generated/kernel_sys_jumptable.inc.h"

#if defined(DEBUG_SYSCALLS)
static const char* sysNames[] = {
#include "syscall_names.inc.h"
};
#endif

// Main syscall dispatcher - routes app calls to kernel implementations
extern "C" uintptr_t KernelSyscallDispatch(uint16_t id,
                                            uintptr_t a0,
                                            uintptr_t a1,
                                            uintptr_t a2,
                                            uintptr_t a3,
                                            uintptr_t a4,
                                            uintptr_t a5,
                                            uintptr_t a6,
                                            uintptr_t a7,
                                            uintptr_t a8,
                                            uintptr_t a9,
                                            uintptr_t a10,
                                            uintptr_t a11,
                                            uintptr_t a12,
                                            uintptr_t a13,
                                            uintptr_t a14,
                                            uintptr_t a15,
                                            uintptr_t a16,
                                            uintptr_t a17,
                                            uintptr_t a18,
                                            uintptr_t a19,
                                            uintptr_t a20,
                                            uintptr_t a21,
                                            uintptr_t a22,
                                            uintptr_t a23,
                                            uintptr_t a24,
                                            uintptr_t a25,
                                            uintptr_t a26,
                                            uintptr_t a27,
                                            uintptr_t a28,
                                            uintptr_t a29) {
#if defined(DEBUG_SYSCALLS)
  const char* name = (id < SYSC__COUNT) ? sysNames[id] : "UNKNOWN";
  Serial.printf("[SYSC] %u (%s) args=[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]\n", id, name,
                a0, a1, a2, a3, a4, a5, a6, a7, a8, a9);
  // Note: Up to 30 args supported, but debug output shows first 10
#endif

  // Jump table dispatch (optimization #3)
  if (id >= SYSC__COUNT) {
#if defined(DEBUG_SYSCALLS)
    Serial.printf("[SYSC] Unknown %u\n", id);
#endif
    return static_cast<uintptr_t>(-1);
  }

  // Direct function pointer lookup and call
  return syscallHandlers[id](a0, a1, a2, a3, a4, a5, a6, a7, a8, a9,
                              a10, a11, a12, a13, a14, a15, a16, a17, a18, a19,
                              a20, a21, a22, a23, a24, a25, a26, a27, a28, a29);
}