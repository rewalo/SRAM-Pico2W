#include <Arduino.h>

#include "generated/raw_data.h"

#define kAppDst (reinterpret_cast<uint8_t*>(0x20030000))
#define kSramSize (0x00070000u)

// ARM interrupt control - disable/enable around memory ops for safety
static inline void cpsid_i() { __asm__("cpsid i"); }
static inline void cpsie_i() { __asm__("cpsie i"); }
// Memory barriers to ensure writes complete before execution
static inline void dsb() { __asm__("dsb 0xF"); }
static inline void isb() { __asm__("isb 0xF"); }

// Copies compiled app binary from flash to SRAM at fixed address
extern "C" void load_app_image_to_sram(void) {
  if (raw_data_len == 0 || raw_data == nullptr) {
    Serial.println("[Kernel] ERROR: Invalid app image");
    return;
  }
  if (raw_data_len > kSramSize) {
    Serial.print("[Kernel] ERROR: App too large (");
    Serial.print(raw_data_len);
    Serial.print(" > ");
    Serial.print(kSramSize);
    Serial.println(" bytes)");
    return;
  }

  // Zero app SRAM region before copying binary to ensure clean state
  size_t zero_size = raw_data_len;
  if (zero_size > kSramSize) {
    zero_size = kSramSize;
  }
  
  uint32_t* zero_ptr = reinterpret_cast<uint32_t*>(kAppDst);
  size_t zero_words = (zero_size + 3) / 4;
  cpsid_i();
  for (size_t i = 0; i < zero_words; i++) {
    zero_ptr[i] = 0;
  }
  cpsie_i();
  dsb();
  isb();

  const uint8_t* src = raw_data;
  uint8_t* dst = kAppDst;
  size_t len = raw_data_len;

  // Align destination to 4 bytes for faster word copies
  while (((reinterpret_cast<uintptr_t>(dst) & 3u) != 0) && len != 0) {
    cpsid_i();
    *dst++ = *src++;
    cpsie_i();
    --len;
  }

  // Fast 32-bit word copies
  const uint32_t* s32 = reinterpret_cast<const uint32_t*>(src);
  uint32_t* d32 = reinterpret_cast<uint32_t*>(dst);
  while (len >= 4) {
    cpsid_i();
    *d32++ = *s32++;
    cpsie_i();
    len -= 4;
  }

  // Copy remaining bytes
  src = reinterpret_cast<const uint8_t*>(s32);
  dst = reinterpret_cast<uint8_t*>(d32);
  while (len-- != 0) {
    cpsid_i();
    *dst++ = *src++;
    cpsie_i();
  }

  // Ensure all writes are visible before jumping to SRAM code
  dsb();
  isb();
}

// Helper to zero a memory region
static void zero_region(uint8_t* start, uint8_t* end, const char* name) {
  if (end <= start) return;
  
  size_t len = end - start;
  uint32_t* ptr32 = reinterpret_cast<uint32_t*>(start);
  size_t words = len / 4;
  size_t rem_bytes = len % 4;
  
  cpsid_i();
  for (size_t i = 0; i < words; i++) {
    ptr32[i] = 0;
  }
  // Zero remaining bytes
  if (rem_bytes > 0) {
    uint8_t* rem = start + (words * 4);
    for (size_t i = 0; i < rem_bytes; i++) {
      rem[i] = 0;
    }
  }
  cpsie_i();
  dsb();
  isb();
}

// Zeros BSS section and ensures .data section is properly initialized
// Must be called after load_app_image_to_sram() and before jumping to app code
extern "C" void zero_app_bss(void) {
  // Read app header to get section boundaries
  struct AppHeader {
    uint32_t magic;
    uint32_t version;
    void (*app_setup)(void);
    void (*app_loop)(void);
    void* syscall_gate;
    void* data_start;
    void* data_end;
    void* bss_start;
    void* bss_end;
    uint32_t* bss_marker;
  };
  
  AppHeader* hdr = reinterpret_cast<AppHeader*>(kAppDst);
  
  // Validate header
  if (hdr->magic != 0x41505041u) {  // "APPA"
    return;  // Invalid header, skip zeroing
  }
  
  // Zero BSS section (uninitialized statics)
  if (hdr->bss_start != nullptr && hdr->bss_end != nullptr) {
    uint8_t* bss_start = reinterpret_cast<uint8_t*>(hdr->bss_start);
    uint8_t* bss_end = reinterpret_cast<uint8_t*>(hdr->bss_end);
    
    // Zero BSS section byte-by-byte
    cpsid_i();
    for (uint8_t* p = bss_start; p < bss_end; p++) {
      *p = 0;
    }
    cpsie_i();
    dsb();
    isb();
    
    // Verify BSS was zeroed by checking the verification marker
    if (hdr->bss_marker != nullptr) {
      uint32_t marker_val = *hdr->bss_marker;
      if (marker_val != 0) {
        Serial.print("[Kernel] ERROR: BSS marker not zero (0x");
        Serial.print(marker_val, HEX);
        Serial.println(") - retrying...");
        // Retry zeroing
        cpsid_i();
        for (uint8_t* p = bss_start; p < bss_end; p++) {
          *p = 0;
        }
        cpsie_i();
        dsb();
        isb();
        marker_val = *hdr->bss_marker;
        if (marker_val != 0) {
          Serial.println("[Kernel] ERROR: BSS zeroing failed!");
        }
      }
    }
    
    Serial.println("[Kernel] BSS zeroed");
  }
}