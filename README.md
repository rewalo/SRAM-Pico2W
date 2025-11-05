# SRAM-Pico2W

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Documentation](https://img.shields.io/badge/Documentation-PDF-red)](docs/technical_documentation.pdf)
[![Platform](https://img.shields.io/badge/Platform-Raspberry%20Pi%20Pico%202W-orange)](https://www.raspberrypi.com/products/raspberry-pi-pico/)

A firmware architecture for Raspberry Pi Pico 2 / Pico 2 W (RP2350) that enables running Arduino-style applications entirely from Static RAM (SRAM) while maintaining a minimal, flash-resident kernel. The system uses a code overlay approach to support applications of unlimited size through demand-loading of code pages.

---

## Table of Contents

- [Quick Start](#quick-start)
- [Project Structure](#project-structure)
- [Key Features](#key-features)
- [Adding New Syscalls](#adding-new-syscalls)
- [Requirements](#requirements)
- [Documentation](#documentation)
- [License](#license)

---

## Quick Start

1. **Build the application:**
   ```batch
   build.bat
   ```

2. **Upload the kernel:**
   - Open `kernel/kernel.ino` in Arduino IDE
   - Select board: **Tools > Board > Raspberry Pi RP2040 Boards > Raspberry Pi Pico 2W**
   - Upload (Ctrl+U)

3. **Monitor output:**
   - Open Serial Monitor (Ctrl+Shift+M)
   - Set baud rate to **115200**

## Project Structure

```
SRAM-Pico2W/
├── app/                             # Application code (SRAM-resident)
│   ├── app.ino                      # Your Arduino sketch
│   ├── src/                         # Application source files
│   │   └── generated/               # Auto-generated syscall wrappers
│   ├── include/                     # Application headers
│   └── linker/                      # Memory map linker script
│
├── kernel/                          # Kernel code (flash-resident)
│   ├── kernel.ino                   # Main kernel file (upload this)
│   └── src/                         # Kernel source files
│       ├── generated/               # Auto-generated dispatch code
│       └── ...                      # Kernel implementation files
│
├── config/                          # Configuration files
│   └── syscalls.def                 # Syscall definitions
│
├── tools/                           # Build tools and utilities
│   ├── syscall_gen.py               # Syscall generator
│   ├── page_gen.py                  # Page map generator for overlay system
│   └── arduino-cli.exe              # Arduino CLI tool
│
├── scripts/                         # Build scripts
│   ├── build_docs.bat               # Documentation builder
│   ├── gen_raw_data.ps1             # Binary to header converter
│   └── technical_documentation.tex  # LaTeX source
│
├── docs/                            # Documentation output
│   └── technical_documentation.pdf
│
└── build.bat                        # Main build script
```

## Key Features

| Feature | Description |
|---------|-------------|
| **SRAM-only applications** | Apps run entirely from SRAM, kernel stays in flash |
| **Code overlay system** | Unlimited application size via demand-loading code pages |
| **Syscall interface** | Controlled hardware access through kernel services |
| **Rapid development** | Only rebuild app, kernel stays in flash |
| **Memory efficient** | ~400 KB available heap, 256 KB code overlay window |

## Adding New Syscalls

1. Edit `config/syscalls.def`
2. Add: `SYSCALL(name, returnType, (args...))`
3. Run `build.bat`
4. Use in your application code

## Requirements

- **Operating System:** Windows (for build scripts)
- **Python:** 3.x
- **Development Environment:** Arduino IDE with RP2040 board support
- **Hardware:** Raspberry Pi Pico 2 / Pico 2 W

## Documentation

[![PDF Documentation](https://bytecode77.com/public/fileicons/pdf.png)](docs/technical_documentation.pdf) **[Technical Documentation](docs/technical_documentation.pdf)**

For complete technical documentation, see the PDF file above. It covers architecture details, syscall interface, memory layout, and implementation guides.

### Rebuilding Documentation

To rebuild the documentation from the LaTeX source:

```batch
scripts\build_docs.bat
```

## License

This project is licensed under the **GNU General Public License v3.0** - see the [LICENSE](LICENSE) file for details.
