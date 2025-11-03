#include <stdint.h>

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

#ifdef __cplusplus
extern "C" {
#endif

extern void app_setup(void);
extern void app_loop(void);

#ifdef __cplusplus
}
#endif

// Section boundaries (defined by linker)
extern char __data_start__[];
extern char __data_end__[];
extern char __bss_start__[];
extern char __bss_end__[];

// BSS verification marker - this MUST be in BSS and MUST be zero after zeroing
// Kernel can check this to verify BSS zeroing worked
static uint32_t __bss_verification_marker;

// App header structure - placed at start of app binary by linker
// Kernel reads this to find setup/loop functions and patches syscall_gate
__attribute__((section(".app_hdr"), used)) struct {
  uint32_t magic;           // "APPA"
  uint32_t version;         // ABI version
  void (*app_setup)(void);
  void (*app_loop)(void);
  SyscallGate syscall_gate;  // Kernel writes this at runtime
  void* data_start;          // Start of .data section
  void* data_end;            // End of .data section
  void* bss_start;           // Start of BSS section (for zeroing)
  void* bss_end;             // End of BSS section (for zeroing)
  uint32_t* bss_marker;       // Pointer to verification marker
} __app_header = {
  0x41505041u, 0x00020000u, app_setup, app_loop, 0,
  __data_start__, __data_end__,
  __bss_start__, __bss_end__,
  &__bss_verification_marker
};

// Returns syscall gate pointer - app's syscall stubs call this
SyscallGate __attribute__((weak)) __syscall_gate_ptr(void) {
  extern typeof(__app_header) __app_header;
  return __app_header.syscall_gate;
}