#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>

#include "generated/raw_data.h"
#include "generated/app_page_map.h"
#include "code_cache.h"

// app_page_map is defined in generated/app_page_map.h

// APP_PAGE_COUNT is defined in app_page_map.h as a macro
// If the header is not included, we provide a default
#ifndef APP_PAGE_COUNT
#define APP_PAGE_COUNT 0
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Code overlay configuration
// Fixed code execution window where all code pages are loaded
// Pages overlay each other in this window - only one set active at a time
#define CODE_OVERLAY_BASE (0x20040000)  // Fixed overlay address (matches linker)
#define CODE_OVERLAY_SIZE (256 * 1024)  // 256KB overlay window
#define CODE_OVERLAY_END (CODE_OVERLAY_BASE + CODE_OVERLAY_SIZE)

// Page size must match page_gen.py
#define PAGE_SIZE 4096
// With overlay, we can have unlimited pages (they overlay each other)
// But we track which pages are currently loaded for LRU eviction
#define MAX_CACHED_PAGES 64  // Track up to 64 pages (can be increased if needed)

// Page cache entry
struct PageCacheEntry {
  uint32_t page_id;
  bool loaded;
  uint32_t cache_addr;  // Address in code cache
  uint32_t access_count;  // For LRU eviction
};

static PageCacheEntry page_cache[MAX_CACHED_PAGES];
static uint32_t loaded_page_count = 0;
// Overlay: all code pages load into the same fixed window
// We track which pages are loaded, but they all use CODE_OVERLAY_BASE


// ARM interrupt control
static inline void cpsid_i() { __asm__("cpsid i"); }
static inline void cpsie_i() { __asm__("cpsie i"); }
static inline void dsb() { __asm__("dsb 0xF"); }
static inline void isb() { __asm__("isb 0xF"); }

// Initialize code overlay
void code_cache_init(void) {
  memset(page_cache, 0, sizeof(page_cache));
  loaded_page_count = 0;
  
  // Zero the overlay region to ensure clean state
  uint32_t* zero_ptr = reinterpret_cast<uint32_t*>(CODE_OVERLAY_BASE);
  uint32_t zero_words = CODE_OVERLAY_SIZE / 4;
  cpsid_i();
  for (uint32_t i = 0; i < zero_words; i++) {
    zero_ptr[i] = 0;
  }
  cpsie_i();
  dsb();
  isb();
  
  // Load critical pages immediately (page 0 = header, page 1 = setup/loop)
  // Note: Header/data pages load at fixed addresses, not in overlay
  for (uint32_t i = 0; i < APP_PAGE_COUNT && i < 2; i++) {
    code_cache_load_page(i, true);
  }
}

// Find cache entry by page ID
static PageCacheEntry* find_cache_entry(uint32_t page_id) {
  for (uint32_t i = 0; i < MAX_CACHED_PAGES; i++) {
    if (page_cache[i].loaded && page_cache[i].page_id == page_id) {
      return &page_cache[i];
    }
  }
  return nullptr;
}

// Find free cache slot
static PageCacheEntry* find_free_slot(void) {
  for (uint32_t i = 0; i < MAX_CACHED_PAGES; i++) {
    if (!page_cache[i].loaded) {
      return &page_cache[i];
    }
  }
  return nullptr;
}

// Evict least recently used page from overlay
static void evict_lru_page(void) {
  PageCacheEntry* lru = nullptr;
  uint32_t min_access = UINT32_MAX;
  
  // Find code page (in overlay) with minimum access count
  // Don't evict header/data pages (they're at fixed addresses)
  // Don't evict critical pages (0-1)
  for (uint32_t i = 0; i < MAX_CACHED_PAGES; i++) {
    PageCacheEntry* entry = &page_cache[i];
    if (entry->loaded && entry->page_id >= 2) {
      const AppPageInfo* page = &app_page_map[entry->page_id];
      // Only evict code pages (section_id == 1) from overlay
      if (page->section_id == 1 && entry->access_count < min_access) {
        min_access = entry->access_count;
        lru = entry;
      }
    }
  }
  
  if (lru) {
    // Mark as unloaded (page will be overwritten when another page loads)
    lru->loaded = false;
    loaded_page_count--;
    
    // Note: We don't zero the overlay region here because:
    // 1. Next page load will overwrite it anyway
    // 2. Zeroing is expensive and not necessary for overlay
    // The overlay naturally gets overwritten when new pages load
  }
}

// Load a page into code overlay window
// All code pages load into the same fixed overlay region, overlaying previous pages
bool code_cache_load_page(uint32_t page_id, bool critical) {
  // Handle case where page map isn't available yet
  if (APP_PAGE_COUNT == 0 || page_id >= APP_PAGE_COUNT) {
    return false;
  }
  
  const AppPageInfo* page_info = &app_page_map[page_id];
  
  // Header and data pages are NOT loaded through overlay - they're at fixed addresses
  // Only code pages (section_id == 1) use the overlay
  if (page_info->section_id == 0 || page_info->section_id == 2) {
    // Header/data pages are loaded directly at fixed addresses by load_app_image_to_sram()
    // Just mark as "loaded" in cache tracking
    PageCacheEntry* existing = find_cache_entry(page_id);
    if (existing) {
      existing->access_count++;
      return true;
    }
    
    // Add to cache tracking (but don't actually load - it's already loaded)
    PageCacheEntry* slot = find_free_slot();
    if (slot) {
      slot->page_id = page_id;
      slot->loaded = true;
      slot->cache_addr = page_info->sram_addr;  // Fixed address
      slot->access_count = 1;
      loaded_page_count++;
    }
    return true;
  }
  
  // Code pages: check if already loaded in overlay
  PageCacheEntry* existing = find_cache_entry(page_id);
  if (existing) {
    existing->access_count++;
    return true;  // Already in overlay
  }
  
  // Evict if cache tracking is full (unless critical)
  if (loaded_page_count >= MAX_CACHED_PAGES && !critical) {
    evict_lru_page();
  }
  
  // Find free slot for tracking
  PageCacheEntry* slot = find_free_slot();
  if (!slot) {
    // Force eviction even for critical pages if absolutely necessary
    evict_lru_page();
    slot = find_free_slot();
    if (!slot) {
      Serial.print("[Overlay] ERROR: No free tracking slots for page ");
      Serial.println(page_id);
      return false;
    }
  }
  
  // CODE OVERLAY: All code pages load into the fixed overlay window
  // Since code is compiled for overlay region (0x20040000), all pages load at base
  // Pages naturally overlay each other - only one set active at a time
  uint32_t sram_addr = CODE_OVERLAY_BASE;
  
  // CRITICAL: Invalidate all other code pages in overlay since they get overwritten
  // Only one code page can be in the overlay at a time
  // Note: We do this BEFORE using slot to avoid invalidating the slot we're about to use
  for (uint32_t i = 0; i < MAX_CACHED_PAGES; i++) {
    PageCacheEntry* entry = &page_cache[i];
    if (entry->loaded && entry->cache_addr >= CODE_OVERLAY_BASE) {
      const AppPageInfo* other_page = &app_page_map[entry->page_id];
      if (other_page->section_id == 1 && entry->page_id != page_id) {  // Code page, not this one
        entry->loaded = false;
        loaded_page_count--;
      }
    }
  }
  
  // Ensure page fits in overlay
  if (page_info->size > CODE_OVERLAY_SIZE) {
    Serial.print("[Overlay] WARNING: Page ");
    Serial.print(page_id);
    Serial.print(" (");
    Serial.print(page_info->size);
    Serial.println(" bytes) exceeds overlay size");
    return false;
  }
  
  // Copy page from flash to SRAM at original address
  const uint8_t* src = raw_data + page_info->flash_offset;
  uint8_t* dst = reinterpret_cast<uint8_t*>(sram_addr);
  uint32_t size = page_info->size;
  
  // Ensure source is valid
  if (page_info->flash_offset + size > raw_data_len) {
    size = raw_data_len - page_info->flash_offset;
    if (size == 0) {
      return false;
    }
  }
  
  // Copy with interrupts disabled for safety
  cpsid_i();
  
  // Align to 4 bytes for faster word copies
  while (((reinterpret_cast<uintptr_t>(dst) & 3u) != 0) && size != 0) {
    *dst++ = *src++;
    --size;
  }
  
  // Fast 32-bit word copies
  const uint32_t* s32 = reinterpret_cast<const uint32_t*>(src);
  uint32_t* d32 = reinterpret_cast<uint32_t*>(dst);
  while (size >= 4) {
    *d32++ = *s32++;
    size -= 4;
  }
  
  // Copy remaining bytes
  src = reinterpret_cast<const uint8_t*>(s32);
  dst = reinterpret_cast<uint8_t*>(d32);
  while (size-- != 0) {
    *dst++ = *src++;
  }
  
  cpsie_i();
  dsb();
  isb();
  
  // Update cache entry - use the slot we found earlier
  // (It's guaranteed to be free since we invalidated other code pages)
  slot->page_id = page_id;
  slot->loaded = true;
  slot->cache_addr = sram_addr;  // Overlay address
  slot->access_count = 1;
  
  loaded_page_count++;
  
  return true;
}

// Get cache address for a page (loads if not present)
uint32_t code_cache_get_addr(uint32_t page_id) {
  if (APP_PAGE_COUNT == 0 || page_id >= APP_PAGE_COUNT) {
    return 0;
  }
  
  PageCacheEntry* entry = find_cache_entry(page_id);
  if (entry) {
    entry->access_count++;
    return entry->cache_addr;
  }
  
  // Load page on demand
  if (code_cache_load_page(page_id, false)) {
    entry = find_cache_entry(page_id);
    if (entry) {
      return entry->cache_addr;
    }
  }
  
  return 0;  // Error
}

// Find page ID for an address
// For overlay: map overlay addresses to page IDs using the page map's sram_addr
uint32_t code_cache_find_page(uint32_t addr) {
  if (APP_PAGE_COUNT == 0) {
    return UINT32_MAX;
  }
  
  // Check if address is in overlay region
  if (addr >= CODE_OVERLAY_BASE && addr < CODE_OVERLAY_END) {
    // Code pages are compiled for overlay region, so their sram_addr in page map
    // matches the overlay addresses. Use this to find which page contains this address.
    for (uint32_t i = 0; i < APP_PAGE_COUNT; i++) {
      const AppPageInfo* page = &app_page_map[i];
      if (page->section_id == 1) {  // Code pages
        // Check if address is in this page's original address range
        if (addr >= page->sram_addr && addr < page->sram_addr + page->size) {
          return i;
        }
      }
    }
  }
  
  // Check fixed address regions (header/data)
  for (uint32_t i = 0; i < APP_PAGE_COUNT; i++) {
    const AppPageInfo* page = &app_page_map[i];
    if (page->section_id == 0 || page->section_id == 2) {
      // Header/data pages at fixed addresses
      if (addr >= page->sram_addr && addr < page->sram_addr + page->size) {
        return i;
      }
    }
  }
  
  return UINT32_MAX;
}

// Preload pages (for optimization)
void code_cache_preload_pages(const uint32_t* page_ids, uint32_t count) {
  if (APP_PAGE_COUNT == 0) {
    return;
  }
  
  for (uint32_t i = 0; i < count; i++) {
    if (page_ids[i] < APP_PAGE_COUNT) {
      code_cache_load_page(page_ids[i], false);
    }
  }
}

#ifdef __cplusplus
}
#endif

