#ifndef __FREERTOS
#define __FREERTOS 1
#endif

#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <Wire.h>
#include <pico/multicore.h>
#include <FreeRTOS.h>
#include <task.h>

#include "generated/syscall_ids.h"
#include "syscall_invoke.h"
#include "syscall_validation.h"
#include "code_cache.h"

//#define DEBUG_SYSCALLS

// =====================================================================
// Core 1 syscall proxy infrastructure
// Core 1 app code cannot safely execute Arduino/FreeRTOS functions
// directly because many of them are only core-0 aware.  We proxy every
// syscall from core 1 through core 0 via a shared request structure and
// a lightweight FIFO notification.
// =====================================================================

struct Core1SyscallMessage {
  volatile uint32_t pending;
  volatile uint16_t id;
  volatile uintptr_t args[30];
  volatile uintptr_t result;
  volatile uint32_t done;
};

static volatile Core1SyscallMessage g_core1_sys_msg = {};
static constexpr uint32_t kCore1SyscallReqMagic = 0xC0DEF1F0;
static constexpr uintptr_t kCore1StatusAddr = 0x2002F000u;
static constexpr bool kCore1StatusEnabled = false;

[[maybe_unused]] inline volatile uint32_t* Core1StatusPtr() {
  return reinterpret_cast<volatile uint32_t*>(kCore1StatusAddr);
}

static uintptr_t DispatchSyscall(uint16_t id, const uintptr_t args[30]);

static uintptr_t ProxySyscallToCore0(uint16_t id, const uintptr_t args[30]);

static void Core1SyscallWorkerTask(void* pv_parameters);

extern "C" void StartCore1SyscallTask(void) {
  xTaskCreate(Core1SyscallWorkerTask, "Core1Sys",
              2048, nullptr, configMAX_PRIORITIES - 1, nullptr);
}

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
}  // namespace syscall_safe_wrappers

// Shared variable for core 1 entry address (accessed from both cores)
// Must be outside namespace for C function access
static volatile uint32_t core1_entry_addr = 0;

// Forward declaration for kernel entry function that runs on core 1
// Must be C linkage for Pico SDK compatibility
extern "C" void core1_kernel_entry(void);

namespace syscall_safe_wrappers {

// Multicore wrapper - validates function pointer is in SRAM before launching
static void multicoreLaunchCore1Safe(uintptr_t entry_addr) {
  if (entry_addr == 0) {
    Serial.println("[Kernel] ERROR: multicore_launch_core1 called with NULL function pointer");
    return;
  }
  // Validate function pointer is in SRAM (app code region)
  if (!syscall_validation::isValidAppPointer(reinterpret_cast<const void*>(entry_addr), 0)) {
    Serial.print("[Kernel] ERROR: multicore_launch_core1 function pointer 0x");
    Serial.print(entry_addr, HEX);
    Serial.println(" is not in SRAM");
    return;
  }
  
  Serial.print("[Kernel] Resetting core 1...");
  Serial.flush();
  
  // Reset core 1 first to ensure clean state
  multicore_reset_core1();
  
  Serial.println(" done");
  Serial.flush();
  
  // Store the entry function address in shared memory
  core1_entry_addr = entry_addr;
  
  volatile uint32_t* core1_flag = nullptr;
  if (kCore1StatusEnabled) {
    core1_flag = Core1StatusPtr();
    *core1_flag = 0;
  }
  
  // Memory barriers
  __asm__ volatile("dmb 0xF" ::: "memory");
  __asm__ volatile("dsb 0xF" ::: "memory");
  __asm__ volatile("isb 0xF" ::: "memory");
  
  Serial.println("[Kernel] Launching core 1...");
  Serial.flush();
  
  // Launch core 1 - use kernel function that's always in flash
  multicore_launch_core1(core1_kernel_entry);
  
  Serial.println("[Kernel] multicore_launch_core1() returned");
  Serial.flush();
  
  // Give core 1 time to start
  delay(100);
  
  if (kCore1StatusEnabled) {
    uint32_t flag_value = *core1_flag;
    if (flag_value == 0xDEADBEEF) {
      Serial.println("[Kernel] Core 1 kernel entry started!");
    } else if (flag_value != 0) {
      Serial.print("[Kernel] Core 1 flag value: 0x");
      Serial.println(flag_value, HEX);
    } else {
      Serial.println("[Kernel] Core 1 flag still 0");
    }
  } else {
    Serial.println("[Kernel] Core 1 telemetry disabled.");
  }
  
  // Wait a bit and check again to see app function progress
  delay(200);
  if (kCore1StatusEnabled) {
    uint32_t flag_value = *core1_flag;
    Serial.print("[Kernel] Core 1 flag after 200ms: 0x");
    Serial.println(flag_value, HEX);
    
    if (flag_value == 0xCAFEBABE) {
      Serial.println("[Kernel] Core 1 is about to call app function");
    } else if (flag_value >= 0x11111111 && flag_value < 0x50000000) {
      Serial.println("[Kernel] Core 1 app function is executing!");
    } else if (flag_value >= 0x40000000) {
      Serial.print("[Kernel] Core 1 is in main loop (counter=");
      Serial.print(flag_value & 0xFFFF);
      Serial.println(")");
    }
  }
  
  Serial.flush();
}

// Kernel function that runs on core 1 - manages app code execution
// This runs in kernel space (flash), so it's always available
// Must be C linkage for Pico SDK multicore_launch_core1()
extern "C" void core1_kernel_entry(void) {
  volatile uint32_t* core1_flag = nullptr;
  if (kCore1StatusEnabled) {
    core1_flag = Core1StatusPtr();
    *core1_flag = 0xDEADBEEF;
  }
  
  // Get the app entry function address from shared memory
  uint32_t entry_addr = core1_entry_addr;
  
  if (entry_addr == 0) {
    // No entry function - just loop
    for (;;) {
      __asm__ volatile("nop");
    }
  }
  
  // Find which page contains the entry function
  uint32_t page_id = code_cache_find_page(entry_addr);
  if (page_id == 0xFFFFFFFF) {
    if (kCore1StatusEnabled && core1_flag != nullptr) {
      *core1_flag = 0xBADBAD00;  // Error: page not found
    }
    for (;;) {
      __asm__ volatile("nop");
    }
  }
  
  // Load the page into overlay window
  if (!code_cache_load_page(page_id, true)) {
    if (kCore1StatusEnabled && core1_flag != nullptr) {
      *core1_flag = 0xBADBAD01;  // Error: failed to load page
    }
    for (;;) {
      __asm__ volatile("nop");
    }
  }
  
  // Update flag to show we're about to call the app function
  if (kCore1StatusEnabled && core1_flag != nullptr) {
    *core1_flag = 0xCAFEBABE;  // Signal: about to call app function
  }
  
  // Cast to function pointer and call the app function
  // The function is now in the overlay window at CODE_OVERLAY_BASE
  void (*entry)(void) = reinterpret_cast<void (*)(void)>(entry_addr);
  
  // Call the app entry function
  // NOTE: This is a chicken-and-egg problem: we need __syscall_raw() loaded,
  // but we don't know which page it's in. The overlay system should ideally
  // handle this automatically, but it doesn't. For now, we'll try loading
  // common pages and hope __syscall_raw() is accessible.
  entry();
  
  // If we return (shouldn't happen for infinite loops), signal error
  if (kCore1StatusEnabled && core1_flag != nullptr) {
    *core1_flag = 0xDEADDEAD;  // Error: function returned unexpectedly
  }
  for (;;) {
    __asm__ volatile("nop");
  }
}
}  // namespace syscall_safe_wrappers

// Include jump table handlers after namespace is defined (they reference syscall_safe_wrappers)
#include "generated/kernel_sys_jumptable.inc.h"

#if defined(DEBUG_SYSCALLS)
static const char* sysNames[] = {
#include "syscall_names.inc.h"
};
#endif

static uintptr_t DispatchSyscall(uint16_t id, const uintptr_t args[30]) {
#if defined(DEBUG_SYSCALLS)
  const char* name = (id < SYSC__COUNT) ? sysNames[id] : "UNKNOWN";
  Serial.printf("[SYSC] %u (%s) args=[%u,%u,%u,%u,%u,%u,%u,%u,%u,%u]\n", id, name,
                args[0], args[1], args[2], args[3], args[4],
                args[5], args[6], args[7], args[8], args[9]);
#endif

  if (id >= SYSC__COUNT) {
#if defined(DEBUG_SYSCALLS)
    Serial.printf("[SYSC] Unknown %u\n", id);
#endif
    return static_cast<uintptr_t>(-1);
  }

  return syscallHandlers[id](args[0], args[1], args[2], args[3], args[4],
                             args[5], args[6], args[7], args[8], args[9],
                             args[10], args[11], args[12], args[13], args[14],
                             args[15], args[16], args[17], args[18], args[19],
                             args[20], args[21], args[22], args[23], args[24],
                             args[25], args[26], args[27], args[28], args[29]);
}

static uintptr_t ProxySyscallToCore0(uint16_t id, const uintptr_t args[30]) {
  while (g_core1_sys_msg.pending) {
    __asm__ volatile("nop");
  }

  g_core1_sys_msg.id = id;
  for (size_t i = 0; i < 30; ++i) {
    g_core1_sys_msg.args[i] = args[i];
  }

  g_core1_sys_msg.result = 0;
  g_core1_sys_msg.done = 0;
  __dmb();
  g_core1_sys_msg.pending = 1;
  __dmb();

  multicore_fifo_push_blocking(kCore1SyscallReqMagic);

  while (!g_core1_sys_msg.done) {
    __asm__ volatile("nop");
  }

  uintptr_t result = g_core1_sys_msg.result;
  g_core1_sys_msg.done = 0;
  return result;
}

static void Core1SyscallWorkerTask(void* /*pv_parameters*/) {
  for (;;) {
    uint32_t msg = multicore_fifo_pop_blocking();
    if (msg != kCore1SyscallReqMagic) {
      continue;
    }

    if (!g_core1_sys_msg.pending) {
      continue;
    }

    uintptr_t args[30];
    for (size_t i = 0; i < 30; ++i) {
      args[i] = g_core1_sys_msg.args[i];
    }

    uintptr_t result = DispatchSyscall(g_core1_sys_msg.id, args);
    g_core1_sys_msg.result = result;
    __dmb();
    g_core1_sys_msg.pending = 0;
    g_core1_sys_msg.done = 1;
  }
}

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
  const uintptr_t args[30] = {
      a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
      a15, a16, a17, a18, a19, a20, a21, a22, a23, a24, a25, a26, a27,
      a28, a29};

  if (get_core_num() == 0) {
    return DispatchSyscall(id, args);
  }

  return ProxySyscallToCore0(id, args);
}