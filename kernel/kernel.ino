#define __FREERTOS 1
#include <Arduino.h>
#include <FreeRTOS.h>
#include <task.h>
#include <stdint.h>

// Syscall gate function type - app calls this to invoke kernel functions
typedef uintptr_t (*SyscallGate)(uint16_t id, uintptr_t a0, uintptr_t a1,
                                  uintptr_t a2, uintptr_t a3, uintptr_t a4,
                                  uintptr_t a5, uintptr_t a6, uintptr_t a7,
                                  uintptr_t a8, uintptr_t a9, uintptr_t a10,
                                  uintptr_t a11, uintptr_t a12, uintptr_t a13,
                                  uintptr_t a14, uintptr_t a15, uintptr_t a16,
                                  uintptr_t a17, uintptr_t a18, uintptr_t a19,
                                  uintptr_t a20, uintptr_t a21, uintptr_t a22,
                                  uintptr_t a23, uintptr_t a24, uintptr_t a25,
                                  uintptr_t a26, uintptr_t a27, uintptr_t a28,
                                  uintptr_t a29);

// App header structure - placed at start of SRAM app image
typedef struct AppHeader {
  uint32_t magic;
  uint32_t version;
  void (*app_setup)(void);
  void (*app_loop)(void);
  SyscallGate syscall_gate;  // Kernel patches this pointer at runtime
  void* data_start;          // Start of .data section
  void* data_end;            // End of .data section
  void* bss_start;           // Start of BSS section (for zeroing)
  void* bss_end;             // End of BSS section (for zeroing)
  uint32_t* bss_marker;      // Pointer to BSS verification marker
  void (**init_array_start)(void);  // Start of .init_array (global constructors)
  void (**init_array_end)(void);    // End of .init_array
  void (**fini_array_start)(void);  // Start of .fini_array (global destructors)
  void (**fini_array_end)(void);     // End of .fini_array
} AppHeader;

#define kAppMagic (0x41505041u)  // "APPA"
#define kAppVersion (0x00020001u)  // Bumped for init_array support
#define kAppBaseAddr (0x20030000u)

extern "C" void load_app_image_to_sram(void);
extern "C" void zero_app_bss(void);
extern "C" void call_app_constructors(void);
extern "C" uintptr_t KernelSyscallDispatch(uint16_t id, uintptr_t a0,
                                            uintptr_t a1, uintptr_t a2,
                                            uintptr_t a3, uintptr_t a4,
                                            uintptr_t a5, uintptr_t a6,
                                            uintptr_t a7, uintptr_t a8,
                                            uintptr_t a9, uintptr_t a10,
                                            uintptr_t a11, uintptr_t a12,
                                            uintptr_t a13, uintptr_t a14,
                                            uintptr_t a15, uintptr_t a16,
                                            uintptr_t a17, uintptr_t a18,
                                            uintptr_t a19, uintptr_t a20,
                                            uintptr_t a21, uintptr_t a22,
                                            uintptr_t a23, uintptr_t a24,
                                            uintptr_t a25, uintptr_t a26,
                                            uintptr_t a27, uintptr_t a28,
                                            uintptr_t a29);
extern "C" void StartCore1SyscallTask(void);

// FreeRTOS task that runs app's loop() repeatedly
static void appTask(void*) {
  AppHeader* hdr = reinterpret_cast<AppHeader*>(kAppBaseAddr);

  for (;;) {
    if (hdr->app_loop != nullptr) {
      hdr->app_loop();
    }
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(10000);
  Serial.println();
  Serial.println("[Kernel] Booting...");

  // Copy app from flash to SRAM
  load_app_image_to_sram();
  Serial.println("[Kernel] Image copied to SRAM");

  // Zero BSS section (uninitialized static variables)
  zero_app_bss();

  // Validate app header
  AppHeader* hdr = reinterpret_cast<AppHeader*>(kAppBaseAddr);
  if (hdr->magic != kAppMagic || hdr->version != kAppVersion) {
    Serial.println("[Kernel] Invalid app header");
    pinMode(LED_BUILTIN, OUTPUT);
    for (;;) {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      delay(150);
    }
  }

  // Patch syscall gate pointer so app can call kernel functions
  hdr->syscall_gate = &KernelSyscallDispatch;

  // Call all global constructors before running setup
  // This initializes all global C++ objects (like String objects)
  call_app_constructors();
  Serial.println("[Kernel] Global constructors called");

  // Run app setup once
  if (hdr->app_setup != nullptr) {
    hdr->app_setup();
  }

  // Launch app loop as separate FreeRTOS task
  xTaskCreate(appTask, "AppTask", 4096, nullptr,
              tskIDLE_PRIORITY + 1, nullptr);
  StartCore1SyscallTask();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(100));
}
