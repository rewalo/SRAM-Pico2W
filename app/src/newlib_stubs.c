#include <stddef.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

// Newlib lock stubs - single-threaded, so no-op
void __retarget_lock_init_recursive(void* lock) {}
void __retarget_lock_close_recursive(void* lock) {}
void __retarget_lock_acquire_recursive(void* lock) {}
void __retarget_lock_release_recursive(void* lock) {}
void __retarget_lock_init(void* lock) {}
void __retarget_lock_close(void* lock) {}
void __retarget_lock_acquire(void* lock) {}
void __retarget_lock_release(void* lock) {}

// Required global lock objects
// These must be non-NULL dummy pointers (newlib checks for NULL)
// Since our lock functions are no-ops, we just need valid non-zero pointers
static char dummy_lock_malloc;
static char dummy_lock_sfp;
void* __lock___malloc_recursive_mutex = &dummy_lock_malloc;
void* __lock___sfp_recursive_mutex = &dummy_lock_sfp;

// Newlib will initialize _impure_ptr and provide localeconv automatically when linked

// POSIX syscall stubs - FILE* operations disabled
int _close(int fd) { return -1; }
int _fstat(int fd, void* st) { return 0; }
int _isatty(int fd) { return 0; }
int _lseek(int fd, int ptr, int dir) { return 0; }
int _read(int fd, void* buf, size_t cnt) { return 0; }
int _write(int fd, const void* b, size_t c) { return (int)c; }
int _kill(int pid, int sig) { return -1; }
int _getpid(void) { return 1; }

// Heap management - provide custom malloc/free AND _sbrk for newlib
// Simple bump allocator (no free tracking, but works for testing)
// Use linker-defined symbols - single source of truth
extern char __heap_start__[];
extern char __heap_end__[];

#define HEAP_START ((char*)__heap_start__)
#define HEAP_END   ((char*)__heap_end__)
#define HEAP_SIZE  (HEAP_END - HEAP_START)

// heapPtr is initialized statically (in .data section)
// Static initialization happens during .data section copy, which occurs BEFORE
// any global constructors run. This ensures the heap is ready when String
// and other global objects need to allocate memory.
static char* heapPtr = HEAP_START;

#ifdef __cplusplus
// Heap initialization constructor with highest priority (runs first)
// This is a safety net to ensure heap is ready before any other constructors.
// With proper global constructor ordering, static initialization should be sufficient,
// but this provides an extra layer of protection.
__attribute__((constructor(1))) static void init_heap_before_all() {
    // Validate and reset heapPtr if corrupted (defensive check)
    if (heapPtr == NULL || heapPtr < HEAP_START || heapPtr > HEAP_END) {
        heapPtr = HEAP_START;
    }
}
#endif

// Override malloc - simple bump allocator
void* malloc(size_t size) {
  if (size == 0) return NULL;
  
  // Defensive check: reset heapPtr if corrupted (protection against memory corruption)
  if (heapPtr == NULL || heapPtr < HEAP_START || heapPtr > HEAP_END) {
    heapPtr = HEAP_START;
  }
  
  // Align to 4 bytes
  size = (size + 3) & ~3;
  
  char* ptr = heapPtr;
  char* next = heapPtr + size;
  
  if (next > HEAP_END) {
    return NULL;  // Out of memory
  }
  
  heapPtr = next;
  return ptr;
}

// Override free - no-op for bump allocator
void free(void* ptr) {
  (void)ptr;  // Bump allocator doesn't track free blocks
}

// Override calloc
void* calloc(size_t nmemb, size_t size) {
  size_t total = nmemb * size;
  void* ptr = malloc(total);
  if (ptr) {
    // Zero the memory
    char* p = (char*)ptr;
    for (size_t i = 0; i < total; i++) {
      p[i] = 0;
    }
  }
  return ptr;
}

// Override realloc - simple implementation
void* realloc(void* ptr, size_t size) {
  if (ptr == NULL) {
    return malloc(size);
  }
  if (size == 0) {
    free(ptr);
    return NULL;
  }
  // For bump allocator, just allocate new block
  // (Can't actually resize in-place)
  void* newPtr = malloc(size);
  if (newPtr && ptr) {
    // Copy old data (but we don't know old size, so this is limited)
    // This is a simplified realloc - not fully correct but works for testing
    char* src = (char*)ptr;
    char* dst = (char*)newPtr;
    for (size_t i = 0; i < size && src + i < heapPtr; i++) {
      dst[i] = src[i];
    }
  }
  return newPtr;
}

// Provide _sbrk for newlib's internal use (even though we override malloc)
void* _sbrk(ptrdiff_t incr) {
  // Defensive check: reset heapPtr if corrupted
  if (heapPtr == NULL || heapPtr < HEAP_START || heapPtr > HEAP_END) {
    heapPtr = HEAP_START;
  }
  
  if (incr == 0) {
    return (void*)heapPtr;
  }
  
  if (incr < 0) {
    return (void*)heapPtr;  // Can't shrink backwards
  }
  
  char* prev = heapPtr;
  char* next = heapPtr + incr;
  
  if (next > HEAP_END) {
    return (void*)-1;  // Out of memory
  }
  
  heapPtr = next;
  return (void*)prev;
}

// Exit handler - just park the CPU
__attribute__((noreturn)) void _exit(int status) {
  (void)status;
  while (1) {
    __asm__ volatile("wfi");
  }
}

__attribute__((weak, noreturn)) void _exit_r(int status) {
  _exit(status);
}

#if defined(__cplusplus)
}
#endif
