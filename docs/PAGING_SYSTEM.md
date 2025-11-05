# Code Paging System

## Overview

The code paging system enables demand-loading of application code pages, allowing larger applications to run within SRAM constraints. Instead of loading the entire application binary at once, the system loads pages on demand as they are needed.

## Architecture

### Page Organization

- **Page Size**: 4KB (4096 bytes)
- **Page Metadata**: Generated at build time from ELF analysis
- **Page Map**: Maps page IDs to flash offsets and SRAM addresses

### Loading Strategy

1. **Immediate Load**: Header and data sections are loaded immediately (they're small and must be at fixed addresses)
2. **Critical Pages**: Setup/loop functions are loaded before app execution
3. **On-Demand Loading**: Other code pages are loaded when first accessed

### Code Cache

- **Cache Size**: 128KB (configurable)
- **LRU Eviction**: Least-recently-used pages are evicted when cache is full
- **Critical Pages**: Pages 0-1 (header, setup/loop) are marked as critical and not evicted

## Code Overlay System

The system uses a **code overlay** approach to support unlimited application size:

### Overlay Architecture

- **Fixed Overlay Window**: 256KB at address `0x20040000`
- **All code pages load into this fixed window**, overlaying previous pages
- **Header/data sections**: Load at fixed addresses (small, must be at specific locations)
- **Code sections**: Load into overlay window on demand

### How It Works

1. **Header and data** are loaded at fixed addresses immediately (they're small)
2. **Code pages** are loaded into the overlay window when needed
3. **Pages overlay each other** - only one set of code pages active at a time
4. **Function calls work** because code is always at the expected overlay address

### Benefits

- ✅ **Unlimited application size** - pages can be larger than SRAM
- ✅ **Simple and stable** - just memory copying, no relocation
- ✅ **No code changes required** - transparent to application
- ✅ **Efficient** - pages swap in/out as needed

### Limitations

- Only one set of code pages can be active at a time (overlay constraint)
- Pages that must be active simultaneously need to fit in overlay window
- Works best for applications with sequential code execution patterns

## Build Process

The build system automatically:

1. Generates page metadata from ELF binary (`page_gen.py`)
2. Creates page map header and implementation files
3. Embeds binary and page metadata into kernel

## Usage

No changes required to application code (`.ino` files). The paging system is transparent to the application.

## Future Enhancements

- [ ] Position-independent code compilation option
- [ ] Runtime code relocation support
- [ ] Code overlay system for unlimited size
- [ ] Preload hints for optimization
- [ ] Page access statistics

