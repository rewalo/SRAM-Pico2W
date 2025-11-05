#pragma once
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize code overlay system
void code_cache_init(void);

// Load a page into cache
bool code_cache_load_page(uint32_t page_id, bool critical);

// Get cache address for a page (loads if not present)
uint32_t code_cache_get_addr(uint32_t page_id);

// Find page ID for an address
uint32_t code_cache_find_page(uint32_t addr);

// Preload pages (for optimization)
void code_cache_preload_pages(const uint32_t* page_ids, uint32_t count);

#ifdef __cplusplus
}
#endif

