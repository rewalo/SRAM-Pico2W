#ifndef __FREERTOS
#define __FREERTOS 1
#endif

#include <Arduino.h>
#include <stdint.h>
#include <SPI.h>
#include <Wire.h>
#include <pico/multicore.h>
#include <pico/bootrom.h>
#include <hardware/watchdog.h>
#include <hardware/gpio.h>
#include <hardware/sync.h>
#include <hardware/structs/ioqspi.h>
#include <hardware/structs/sio.h>
#include <FreeRTOS.h>
#include <task.h>

// Stub out Ethernet async-context functions that lwIP_Ethernet expects
// These are required by lwIP_Ethernet but not available in FreeRTOS builds
// We provide minimal stubs to satisfy the linker
// Note: These are Pico SDK async_context functions that Ethernet library needs
extern "C" {
  // Stub: Return default config (we don't use Ethernet, so return nullptr)
  void* async_context_threadsafe_background_default_config(void) {
    return nullptr;
  }
  
  // Stub: Initialize async context (we don't use Ethernet, so return nullptr)
  void* async_context_threadsafe_background_init(void* config) {
    (void)config;  // Suppress unused parameter warning
    return nullptr;
  }
}

#include <WiFi.h>
// Intentionally do NOT include <WiFiServer.h>/<WiFiClient.h>/<WiFiUdp.h> here.
// Those headers can pull in lwIP Ethernet async-context glue which conflicts with
// this FreeRTOS-based kernel build in some Arduino-Pico configurations.
// SerialBT requires Bluetooth to be enabled in IDE settings.
// Only include it when the core sets ENABLE_CLASSIC=1 (Tools -> IP/Bluetooth Stack).
#if defined(ENABLE_CLASSIC) && ENABLE_CLASSIC
#include <SerialBT.h>
#define SRAM_PICO2W_HAS_SERIALBT 1
#else
#define SRAM_PICO2W_HAS_SERIALBT 0
#endif

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

// BOOTSEL button check - reads the bootsel button state.
// Based on Arduino Pico's implementation, but adapted for syscall context.
// The original uses rp2040.idleOtherCore() which can deadlock when called
// from core 1 via syscall proxy. This version works safely from core 0.
// Must be in global namespace for syscall generator.
static bool __no_inline_not_in_flash_func(GetBootselButtonSafe)() {
  constexpr uint32_t kCsPinIndex = 1;
  constexpr int kSettleDelay = 1000;
  
  // Since we're executing on core 0 (via syscall proxy), we can safely
  // access flash CS pin. We skip rp2040.idleOtherCore() because:
  // 1. If called from core 0 directly, it's not needed.
  // 2. If called from core 1 via proxy, we're already on core 0.
  // 3. FreeRTOS context might have issues with core idling.
  
  // Disable interrupts - critical section for flash access.
  noInterrupts();
  
  // Set chip select to Hi-Z (floating) to read button state.
  hw_write_masked(&ioqspi_hw->io[kCsPinIndex].ctrl,
                  GPIO_OVERRIDE_LOW << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                  IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
  
  // Small delay to let pin settle.
  for (volatile int i = 0; i < kSettleDelay; ++i) {
    // Busy wait.
  }
  
  // Read button state - button pulls pin LOW when pressed.
  // Use SIO GPIO HI register to read QSPI CS pin.
#if PICO_RP2040
  constexpr uint32_t kCsBit = (1u << 1);
#else
  constexpr uint32_t kCsBit = SIO_GPIO_HI_IN_QSPI_CSN_BITS;
#endif
  const bool button_pressed = !(sio_hw->gpio_hi_in & kCsBit);
  
  // Restore chip select to normal operation.
  hw_write_masked(&ioqspi_hw->io[kCsPinIndex].ctrl,
                  GPIO_OVERRIDE_NORMAL << IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_LSB,
                  IO_QSPI_GPIO_QSPI_SS_CTRL_OEOVER_BITS);
  
  // Re-enable interrupts.
  interrupts();
  
  return button_pressed;
}

bool bootselButton() {
  // Call the safe implementation that works from syscall context.
  // This runs on core 0 (where syscalls execute), so it's safe.
  return GetBootselButtonSafe();
}

// Software reset - resets the device
// Must be in global namespace for syscall generator
void softwareReset() {
  watchdog_reboot(0, 0, 0);  // Use watchdog to reset
}

// =====================================================================
// Interrupt support
// =====================================================================

// Interrupt mode constants (matching Arduino)
#define CHANGE  0x01
#define FALLING 0x02
#define RISING  0x03

// Storage for ISR function pointers (max 30 GPIO pins)
static void (*isr_handlers[30])(void) = {nullptr};

// GPIO interrupt callback - dispatches to app ISR
static void gpio_isr_callback(uint gpio, uint32_t events) {
  if (gpio < 30 && isr_handlers[gpio] != nullptr) {
    // Validate ISR pointer is in SRAM (app code region)
    uintptr_t isr_addr = reinterpret_cast<uintptr_t>(isr_handlers[gpio]);
    if (syscall_validation::isValidAppPointer(reinterpret_cast<const void*>(isr_addr), 0)) {
      // Call the app ISR
      isr_handlers[gpio]();
    }
  }
}

// Attach interrupt handler to GPIO pin
// NOTE: Must NOT be named attachInterrupt, because Arduino core provides overloads and
// the generated jump table needs an unambiguous function pointer.
void KernelAttachInterrupt(uint8_t pin, uintptr_t isr_addr, int mode) {
  if (pin >= 30) {
    Serial.print("[Kernel] ERROR: attachInterrupt pin ");
    Serial.print(pin);
    Serial.println(" out of range");
    return;
  }
  
  // Validate ISR pointer is in SRAM
  if (!syscall_validation::isValidAppPointer(reinterpret_cast<const void*>(isr_addr), 0)) {
    Serial.print("[Kernel] ERROR: attachInterrupt ISR pointer 0x");
    Serial.print(isr_addr, HEX);
    Serial.println(" is not in SRAM");
    return;
  }
  
  // Convert Arduino interrupt mode to Pico SDK mode
  uint32_t pico_mode = 0;
  if (mode == RISING) {
    pico_mode = GPIO_IRQ_EDGE_RISE;
  } else if (mode == FALLING) {
    pico_mode = GPIO_IRQ_EDGE_FALL;
  } else if (mode == CHANGE) {
    pico_mode = GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL;
  } else {
    Serial.print("[Kernel] ERROR: attachInterrupt invalid mode ");
    Serial.println(mode);
    return;
  }
  
  // Store ISR handler
  isr_handlers[pin] = reinterpret_cast<void (*)(void)>(isr_addr);
  
  // Configure GPIO interrupt
  gpio_set_irq_enabled_with_callback(pin, pico_mode, true, gpio_isr_callback);
}

// Detach interrupt handler from GPIO pin
// See note above about naming.
void KernelDetachInterrupt(uint8_t pin) {
  if (pin >= 30) {
    return;
  }
  
  // Disable GPIO interrupt
  gpio_set_irq_enabled(pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
  
  // Clear ISR handler
  isr_handlers[pin] = nullptr;
}

// =====================================================================
// WiFi support wrappers
// =====================================================================

namespace syscall_safe_wrappers {
// WiFi begin with password
static int wifiBegin(const char* ssid, const char* password) {
  if (!syscall_validation::isValidAppString(ssid)) {
    Serial.println("[Kernel] ERROR: Invalid SSID pointer in WiFi.begin");
    return WL_CONNECT_FAILED;
  }
  if (password != nullptr && !syscall_validation::isValidAppString(password)) {
    Serial.println("[Kernel] ERROR: Invalid password pointer in WiFi.begin");
    return WL_CONNECT_FAILED;
  }
  return WiFi.begin(ssid, password);
}

// WiFi begin open network
static int wifiBeginOpen(const char* ssid) {
  if (!syscall_validation::isValidAppString(ssid)) {
    Serial.println("[Kernel] ERROR: Invalid SSID pointer in WiFi.begin");
    return WL_CONNECT_FAILED;
  }
  return WiFi.begin(ssid);
}

// WiFi status - returns uint8_t, but syscall expects int
static int wifiStatus() {
  return static_cast<int>(WiFi.status());
}

// WiFi disconnect - takes optional bool parameter, but syscall has none
static int wifiDisconnect() {
  return WiFi.disconnect();  // Calls disconnect(false)
}

// WiFi localIP - returns IPAddress, convert to uint32_t
static uint32_t wifiLocalIP() {
  return WiFi.localIP();
}

// WiFi subnetMask - returns IPAddress, convert to uint32_t
static uint32_t wifiSubnetMask() {
  return WiFi.subnetMask();
}

// WiFi gatewayIP - returns IPAddress, convert to uint32_t
static uint32_t wifiGatewayIP() {
  return WiFi.gatewayIP();
}

// WiFi SSID getter - returns pointer to internal buffer (safe for app to read)
static const char* wifiSSID() {
  return WiFi.SSID().c_str();
}

// WiFi hostname setter
static int wifiHostname(const char* hostname) {
  if (!syscall_validation::isValidAppString(hostname)) {
    Serial.println("[Kernel] ERROR: Invalid hostname pointer in WiFi.hostname");
    return 0;
  }
  WiFi.hostname(hostname);  // Returns void, not int
  return 1;  // Return success
}

// WiFi hostname getter
static const char* wifiGetHostname() {
  return WiFi.getHostname();  // Already returns const char*, not String
}

// WiFi MAC address getter - copies to app buffer
static uint8_t* wifiMacAddress(uint8_t* mac) {
  if (!syscall_validation::isValidAppPointer(mac, 6)) {
    Serial.println("[Kernel] ERROR: Invalid MAC buffer pointer in WiFi.macAddress");
    return nullptr;
  }
  uint8_t* kernel_mac = WiFi.macAddress(mac);
  // Copy to app buffer if kernel returned different pointer
  if (kernel_mac != mac) {
    memcpy(mac, kernel_mac, 6);
  }
  return mac;
}
}  // namespace syscall_safe_wrappers

// =====================================================================
// Bluetooth/SerialBT support wrappers
// =====================================================================
// Note: Pico W/2W uses SerialBT for Bluetooth Classic, not ArduinoBLE
// SerialBT provides serial port profile (SPP) functionality

namespace syscall_safe_wrappers {
// SerialBT begin - starts Bluetooth Classic
// Note: SerialBT.begin() returns void, not bool
static bool serialBTBegin() {
  #if SRAM_PICO2W_HAS_SERIALBT
  SerialBT.begin();
  return true;
  #else
  Serial.println("[Kernel] ERROR: SerialBT requires Bluetooth to be enabled in IDE settings");
  return false;
  #endif
}

// SerialBT end - stops Bluetooth Classic
static void serialBTEnd() {
  #if SRAM_PICO2W_HAS_SERIALBT
  SerialBT.end();
  #endif
}

// SerialBT available - checks if data is available
static bool serialBTAvailable() {
  #if SRAM_PICO2W_HAS_SERIALBT
  return SerialBT.available() > 0;
  #else
  return false;
  #endif
}

// SerialBT connected - checks if connected
// Note: SerialBT doesn't have hasClient(), use available() as proxy
static bool serialBTConnected() {
  #if SRAM_PICO2W_HAS_SERIALBT
  return SerialBT.available() > 0;  // Approximate - SerialBT doesn't expose connection state directly
  #else
  return false;
  #endif
}

// SerialBT disconnect - disconnects client
// Note: SerialBT doesn't have disconnect(), use end() instead
static void serialBTDisconnect() {
  #if SRAM_PICO2W_HAS_SERIALBT
  SerialBT.end();  // End connection
  #endif
}

// SerialBT address - gets MAC address (returns static buffer)
static const char* serialBTAddress() {
  // SerialBT doesn't have a direct address() method
  // Use WiFi MAC address as Bluetooth uses the same chip
  static char mac_str[18];
  uint8_t mac[6];
  WiFi.macAddress(mac);
  sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
          mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return mac_str;
}

// SerialBT advertise (stub - SerialBT doesn't have explicit advertise)
static void serialBTAdvertise() {
  // SerialBT automatically advertises when begin() is called
  // This is a no-op for compatibility
}

// SerialBT stop advertise (stub)
static void serialBTStopAdvertise() {
  // SerialBT doesn't have explicit stop advertise
  // This is a no-op for compatibility
}

// SerialBT set advertised service UUID (not available in SerialBT, stub)
static void bleSetAdvertisedServiceUuid(const char* uuid) {
  // SerialBT doesn't support BLE advertising - this is a stub
  (void)uuid;  // Suppress unused parameter warning
}

// SerialBT set advertised service data (not available in SerialBT, stub)
static void bleSetAdvertisedServiceData(uint16_t uuid, const uint8_t* data, uint8_t length) {
  // SerialBT doesn't support BLE advertising - this is a stub
  (void)uuid;
  (void)data;
  (void)length;
}

// SerialBT set local name (not directly supported, stub)
static void bleSetLocalName(const char* name) {
  // SerialBT doesn't have a direct setLocalName - this is a stub
  (void)name;
}

// SerialBT address getter
static const char* bleAddress() {
  return serialBTAddress();
}

// SerialBT central (not applicable for SerialBT, always returns false)
static bool serialBTCentral() {
  // SerialBT acts as a peripheral, not central
  return false;
}
}  // namespace syscall_safe_wrappers

// Global BLE functions that map to SerialBT
// Must be in global namespace for syscall generator
bool BLE_begin() {
  return syscall_safe_wrappers::serialBTBegin();
}

void BLE_end() {
  syscall_safe_wrappers::serialBTEnd();
}

void BLE_advertise() {
  syscall_safe_wrappers::serialBTAdvertise();
}

void BLE_stopAdvertise() {
  syscall_safe_wrappers::serialBTStopAdvertise();
}

bool BLE_available() {
  return syscall_safe_wrappers::serialBTAvailable();
}

bool BLE_connected() {
  return syscall_safe_wrappers::serialBTConnected();
}

void BLE_disconnect() {
  syscall_safe_wrappers::serialBTDisconnect();
}

bool BLE_central() {
  return syscall_safe_wrappers::serialBTCentral();
}

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
#include "generated/syscall_names.inc.h"
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