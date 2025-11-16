#pragma once

#ifdef __cplusplus

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
// Include Arduino libraries BEFORE stdlib.h to avoid POSIX random() conflict
#include "WMath.h"
#include "WCharacter.h"
#include <stdlib.h>
#include <ctype.h>

#include "app_syscalls.h"
#include "WString.h"

// Arduino-style Serial proxy - forwards calls to syscall wrappers
struct SerialProxy {
  inline void begin(unsigned long baud) { Serial_begin(baud); }
  inline size_t write(uint8_t b) { return Serial_write_b(b); }
  inline void print(const char* s) { Serial_print_s(s); }
  inline void println(const char* s) { Serial_println_s(s); }
  inline void println() { Serial_println(); }
  inline int available() { return Serial_available(); }
  inline int read() { return Serial_read(); }
  inline int peek() { return Serial_peek(); }
  inline void flush() { Serial_flush(); }

  // Integer to decimal string conversion
  static inline void itoa10(int v, char* b) {
    char* p = b;
    if (v == 0) {
      *p++ = '0';
      *p = 0;
      return;
    }
    bool neg = v < 0;
    unsigned n = neg ? static_cast<unsigned>(-v) : static_cast<unsigned>(v);
    char tmp[12];
    unsigned i = 0;
    while (n != 0 && i < sizeof(tmp)) {
      tmp[i++] = '0' + (n % 10);
      n /= 10;
    }
    if (neg) *p++ = '-';
    while (i != 0) *p++ = tmp[--i];
    *p = 0;
  }

  inline void print(int v) {
    char buf[16];
    itoa10(v, buf);
    Serial_print_s(buf);
  }
  inline void println(int v) {
    char buf[16];
    itoa10(v, buf);
    Serial_println_s(buf);
  }
  
  inline void print(unsigned int v) {
    char buf[16];
    // Convert unsigned int to string
    char* p = buf;
    if (v == 0) {
      *p++ = '0';
      *p = 0;
    } else {
      char tmp[12];
      unsigned i = 0;
      unsigned n = v;
      while (n != 0 && i < sizeof(tmp)) {
        tmp[i++] = '0' + (n % 10);
        n /= 10;
      }
      while (i != 0) *p++ = tmp[--i];
      *p = 0;
    }
    Serial_print_s(buf);
  }
  
  inline void println(unsigned int v) {
    print(v);
    Serial_println();
  }

  // Long support - use sprintf for reliable formatting.
  // Static buffer ensures valid memory location for kernel pointer validation.
  inline void print(long v) {
    static char buf[32];
    sprintf(buf, "%ld", v);
    Serial_print_s(buf);
  }
  inline void println(long v) {
    print(v);
    Serial_println();
  }

  // Char support.
  // Static buffer ensures valid memory location for kernel pointer validation.
  inline void print(char c) {
    static char buf[2];
    buf[0] = c;
    buf[1] = 0;
    Serial_print_s(buf);
  }
  inline void println(char c) {
    print(c);
    Serial_println();
  }

  // String support - defined after String class
  inline void print(const String& s);
  inline void println(const String& s);
};

// Global Serial object for app code
inline SerialProxy Serial;

// Arduino-style SPI proxy - forwards calls to syscall wrappers
struct SPIProxy {
  inline void begin() { SPI_begin(); }
  inline void end() { SPI_end(); }
  inline void beginTransaction() { SPI_beginTransaction(); }
  inline void endTransaction() { SPI_endTransaction(); }
  inline uint8_t transfer(uint8_t data) { return SPI_transfer(data); }
};

// Global SPI object for app code
inline SPIProxy SPI;

// Arduino-style Wire (I2C) proxy - forwards calls to syscall wrappers
struct WireProxy {
  inline void begin() { Wire_begin(); }
  inline void end() { Wire_end(); }
  inline void beginTransmission(uint8_t address) { Wire_beginTransmission(address); }
  inline uint8_t endTransmission() { return Wire_endTransmission(); }
  inline uint8_t requestFrom(uint8_t address, uint8_t quantity) { return Wire_requestFrom(address, quantity); }
  inline size_t write(uint8_t data) { return Wire_write_b(data); }
  inline size_t write(const uint8_t* data, size_t length) { return Wire_write_buf(data, length); }
  inline int available() { return Wire_available(); }
  inline int read() { return Wire_read(); }
};

// Global Wire object for app code
inline WireProxy Wire;

// Serial String support - implement after String is defined
inline void SerialProxy::print(const String& s) {
  Serial_print_s(s.c_str());
}

inline void SerialProxy::println(const String& s) {
  Serial_println_s(s.c_str());
}

#endif