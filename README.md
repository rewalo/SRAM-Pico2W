# SRAM-Pico2W

A firmware architecture for Raspberry Pi Pico 2 / Pico 2 W (RP2350) that enables running Arduino-style applications entirely from Static RAM (SRAM) while maintaining a minimal, flash-resident kernel.

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
│   ├── include/                     # Application headers
│   ├── generated/                   # Auto-generated syscall wrappers
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

- **SRAM-only applications:** Apps run entirely from SRAM, kernel stays in flash
- **Syscall interface:** Controlled hardware access through kernel services
- **Rapid development:** Only rebuild app, kernel stays in flash
- **Memory efficient:** ~400 KB available heap for applications

## Adding New Syscalls

1. Edit `config/syscalls.def`
2. Add: `SYSCALL(name, returnType, (args...))`
3. Run `build.bat`
4. Use in your application code

## Requirements

- Windows (for build scripts)
- Python 3.x
- Arduino IDE with RP2040 board support
- Raspberry Pi Pico 2 / Pico 2 W

## Documentation

See `docs/technical_documentation.pdf` for complete technical documentation.

To rebuild documentation:
```batch
scripts\build_docs.bat
```