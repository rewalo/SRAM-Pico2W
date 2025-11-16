@echo off
setlocal EnableExtensions EnableDelayedExpansion

:: Build script for SRAM app - compiles app binary and converts to raw_data.h
:: Builds app with minimal libc support, then embeds binary in kernel

set BUILD_START=%time%
echo ===========================================================================
echo   SRAM-Pico2W Build Script
echo ===========================================================================
echo.

:: ===========================================================================
::  BUILD CONFIGURATION
:: ===========================================================================
echo [CONFIG] Loading build configuration...
echo.

:: Libc linking toggle
set LIBC=ON

:: Custom libraries - add paths here, e.g.:
:: set CUSTOM_LIBS=-L"%APP%\libs\mylib" -lmylib
set CUSTOM_LIBS=

:: Custom include directories - add paths here, e.g.:
:: set CUSTOM_INCLUDES=-I"%APP%\libs\mylib\include"
set CUSTOM_INCLUDES=

:: Optimization level (Os = size, O2 = speed, O0 = debug)
set OPT_LEVEL=Os

:: Enable verbose output (ON/OFF)
set VERBOSE=OFF

echo   Configuration:
echo     - LIBC:           %LIBC%
echo     - Optimization:   %OPT_LEVEL%
echo     - Custom Libs:    %CUSTOM_LIBS%
echo     - Custom Includes:%CUSTOM_INCLUDES%
echo.

:: ===========================================================================
::  PATH SETUP
:: ===========================================================================
set "ROOT=%~dp0"
if "%ROOT:~-1%"=="\" set "ROOT=%ROOT:~0,-1%"

set "APP=%ROOT%\app"
set "KERNEL=%ROOT%\kernel"
set "TOOLS=%ROOT%\tools"
set "SCRIPTS=%ROOT%\scripts"
set "CONFIG=%ROOT%\config"
set "LINKER=%APP%\linker\memmap_app_ram.ld"
set "TEMP=%APP%\temp"
set "BUILD=%APP%\build"

if not exist "%TEMP%" mkdir "%TEMP%"
if not exist "%BUILD%" mkdir "%BUILD%"

:: ===========================================================================
::  STEP 1: GENERATE SYSCALL WRAPPERS
:: ===========================================================================
echo [STEP 1/4] Generating syscall wrappers...
python "%TOOLS%\syscall_gen.py"
if errorlevel 1 (
  echo.
  echo [ERROR] syscall_gen.py failed!
  goto FAIL
)
echo   [OK] Syscall wrappers generated
echo.

:: ===========================================================================
::  STEP 2: LOCATE TOOLCHAIN
:: ===========================================================================
echo [STEP 2/4] Locating toolchain...

set "TCROOT=%LocalAppData%\Arduino15\packages\rp2040\tools\pqt-gcc"
for /d %%D in ("%TCROOT%\*") do set "TCDIR=%%~fD"
set "BIN=%TCDIR%\bin"
set "LIBDIR=%TCDIR%\lib\gcc\arm-none-eabi"
set "NEWLIBDIR=%TCDIR%\arm-none-eabi\lib"
set "NEWLIBINCDIR=%TCDIR%\arm-none-eabi\include"
set "GCCINCDIR=%TCDIR%\lib\gcc\arm-none-eabi"
:: Find GCC version directory
for /d %%V in ("%GCCINCDIR%\*") do set "GCCVERDIR=%%~fV"

if not exist "%BIN%\arm-none-eabi-g++.exe" (
  echo [ERROR] RP2040 toolchain not found in %TCROOT%
  goto FAIL
)

echo   [OK] Toolchain found: %TCDIR%
echo.

:: ===========================================================================
::  STEP 3: COMPILATION
:: ===========================================================================
echo [STEP 3/4] Compiling app...

:: Compiler flags
if /I "%LIBC%"=="ON" (
  set CFLAGS=-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -DNO_STDIO -%OPT_LEVEL% -g0 ^
  -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti -fno-common ^
  -ffast-math -fno-math-errno   -fno-unwind-tables -fno-asynchronous-unwind-tables ^
  -I"%APP%" ^
  -I"%APP%\src" ^
  -I"%APP%\src\generated" ^
  -I"%APP%\include" ^
  -I"%NEWLIBINCDIR%" ^
  -isystem "!GCCVERDIR!\include" ^
  -isystem "!GCCVERDIR!\include-fixed" ^
  %CUSTOM_INCLUDES% ^
  -include "%APP%\include\common.h"
) else (
  set CFLAGS=-mcpu=cortex-m33 -mthumb -mfloat-abi=softfp -ffreestanding -DNO_STDIO -%OPT_LEVEL% -g0 ^
  -fdata-sections -ffunction-sections -fno-exceptions -fno-rtti -fno-common ^
  -ffast-math -fno-math-errno -fno-unwind-tables -fno-asynchronous-unwind-tables ^
  -I"%APP%" ^
  -I"%APP%\src" ^
  -I"%APP%\src\generated" ^
  -I"%APP%\include" ^
  %CUSTOM_INCLUDES% ^
  -include "%APP%\include\common.h"
)

:: Linker flags
set LDFLAGS_BASE=-nostdlib -nostartfiles -Wl,--gc-sections -Wl,--strip-all -Wl,--strip-debug ^
-Wl,--build-id=none -Wl,--no-keep-memory -Wl,--no-warn-rwx-segments ^
-T"%LINKER%"

if /I "%LIBC%"=="ON" (
    echo   [INFO] LIBC linking enabled
    :: Find libgcc directory (version-specific)
    for /d %%D in ("%LIBDIR%\*") do set "LIBGCCDIR=%%~fD"
    :: Force link libc by requiring malloc symbol, use --start-group/--end-group for circular deps
    set LDFLAGS=%LDFLAGS_BASE% -L"%NEWLIBDIR%" -L"!LIBGCCDIR!" -Wl,--start-group -lc -lm -lgcc -Wl,--end-group -Wl,-Map="%BUILD%\app.map" %CUSTOM_LIBS%
) else (
    echo   [INFO] LIBC linking disabled
    set LDFLAGS=%LDFLAGS_BASE% -Wl,--discard-all %CUSTOM_LIBS%
)

if /I "%VERBOSE%"=="ON" (
  echo   Compiling: app.ino
)
"%BIN%\arm-none-eabi-g++.exe" ^
  -include "%APP%\src\generated\app_syscalls.h" ^
  -include "%APP%\include\arduino_proxies.h" ^
  -x c++ -c "%APP%\app.ino" -o "%TEMP%\app.o" %CFLAGS% || goto FAIL

if /I "%VERBOSE%"=="ON" (
  echo   Compiling: support files
)
"%BIN%\arm-none-eabi-gcc.exe" -c "%APP%\src\app_header.c" -o "%TEMP%\app_header.o" %CFLAGS% || goto FAIL
"%BIN%\arm-none-eabi-gcc.exe" -c "%APP%\src\generated\app_sys_raw.c" -o "%TEMP%\app_sys_raw.o" %CFLAGS% || goto FAIL
"%BIN%\arm-none-eabi-g++.exe" -c "%APP%\src\app_export.c" -o "%TEMP%\app_export.o" %CFLAGS% || goto FAIL
"%BIN%\arm-none-eabi-g++.exe" -c "%APP%\src\WString.cpp" -o "%TEMP%\WString.o" %CFLAGS% || goto FAIL
"%BIN%\arm-none-eabi-g++.exe" -c "%APP%\src\WMath.cpp" -o "%TEMP%\WMath.o" %CFLAGS% || goto FAIL
"%BIN%\arm-none-eabi-g++.exe" -c "%APP%\src\arduino_utils.cpp" -o "%TEMP%\arduino_utils.o" %CFLAGS% || goto FAIL

if /I "%LIBC%"=="ON" (
  if /I "%VERBOSE%"=="ON" (
    echo   Compiling: newlib_stubs.c
  )
  "%BIN%\arm-none-eabi-gcc.exe" -c "%APP%\src\newlib_stubs.c" -o "%TEMP%\newlib_stubs.o" %CFLAGS% || goto FAIL
  set LIBC_OBJ=%TEMP%\newlib_stubs.o
) else (
  set LIBC_OBJ=
)

if /I "%VERBOSE%"=="ON" (
  echo   Linking: app.elf
)
if /I "%LIBC%"=="ON" (
  "%BIN%\arm-none-eabi-g++.exe" ^
    "%TEMP%\app.o" ^
    "%TEMP%\app_header.o" ^
    "%TEMP%\app_sys_raw.o" ^
    "%TEMP%\app_export.o" ^
    "%TEMP%\WString.o" ^
    "%TEMP%\WMath.o" ^
    "%TEMP%\arduino_utils.o" ^
    %LIBC_OBJ% ^
    -o "%BUILD%\app.elf" %CFLAGS% %LDFLAGS% || goto FAIL
) else (
  "%BIN%\arm-none-eabi-g++.exe" ^
    "%TEMP%\app.o" ^
    "%TEMP%\app_header.o" ^
    "%TEMP%\app_sys_raw.o" ^
    "%TEMP%\app_export.o" ^
    "%TEMP%\WString.o" ^
    "%TEMP%\WMath.o" ^
    "%TEMP%\arduino_utils.o" ^
    -o "%BUILD%\app.elf" %CFLAGS% %LDFLAGS% || goto FAIL
)

if /I "%VERBOSE%"=="ON" (
  echo   Converting: app.elf -> app.bin
)
"%BIN%\arm-none-eabi-objcopy.exe" -O binary "%BUILD%\app.elf" "%BUILD%\app.bin" || goto FAIL

:: Generate page metadata from ELF
if /I "%VERBOSE%"=="ON" (
  echo   Generating: page metadata
)
python "%TOOLS%\page_gen.py" "%BUILD%\app.elf" "%BUILD%\app.bin" "%KERNEL%\src\generated" || goto FAIL

for %%F in ("%BUILD%\app.bin") do set BIN_SIZE=%%~zF
if !BIN_SIZE! GEQ 1024 (
  set /a BIN_SIZE_KB=!BIN_SIZE! / 1024
  set /a BIN_SIZE_REM=!BIN_SIZE! %% 1024
  set /a BIN_SIZE_DEC=!BIN_SIZE_REM! * 100 / 1024
  if !BIN_SIZE_DEC! LSS 10 (
    echo   [OK] Binary size: !BIN_SIZE_KB!.0!BIN_SIZE_DEC! KB ^(!BIN_SIZE! bytes^)
  ) else (
    echo   [OK] Binary size: !BIN_SIZE_KB!.!BIN_SIZE_DEC! KB ^(!BIN_SIZE! bytes^)
  )
) else (
  echo   [OK] Binary size: !BIN_SIZE! bytes
)
echo.

:: ===========================================================================
::  STEP 4: GENERATE raw_data.h
:: ===========================================================================
echo [STEP 4/4] Generating raw_data.h...

if not exist "%BUILD%\app.bin" (
  echo [ERROR] app.bin not found at "%BUILD%\app.bin"
  goto FAIL
)

powershell -NoProfile -ExecutionPolicy Bypass -File "%SCRIPTS%\gen_raw_data.ps1" -BinPath "%BUILD%\app.bin" -OutDir "%KERNEL%\src\generated"

if errorlevel 1 goto FAIL

echo.

:: ===========================================================================
::  CLEANUP
:: ===========================================================================
if /I "%VERBOSE%"=="ON" (
  echo [CLEANUP] Removing temporary files...
)
del "%TEMP%\*.o" >nul 2>&1
rmdir "%TEMP%" >nul 2>&1
rmdir /S /Q "%BUILD%" >nul 2>&1

set BUILD_END=%time%

powershell -NoProfile -Command "$startStr = '%BUILD_START%'.Trim(); $endStr = '%time%'.Trim(); $start = [DateTime]::ParseExact($startStr, 'H:mm:ss.ff', $null); $end = [DateTime]::ParseExact($endStr, 'H:mm:ss.ff', $null); $elapsed = ($end - $start).TotalSeconds; Write-Host ('Build SUCCESS (took ' + $elapsed.ToString('0.00') + 's)')"

echo.
pause
exit /b 0

:FAIL
set BUILD_END=%time%
powershell -NoProfile -Command "$startStr = '%BUILD_START%'.Trim(); $endStr = '%time%'.Trim(); $start = [DateTime]::ParseExact($startStr, 'H:mm:ss.ff', $null); $end = [DateTime]::ParseExact($endStr, 'H:mm:ss.ff', $null); $elapsed = ($end - $start).TotalSeconds; Write-Host ('Build FAILED (took ' + $elapsed.ToString('0.00') + 's)')"
echo.
pause
exit /b 1
