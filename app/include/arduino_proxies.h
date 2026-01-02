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

// Multicore helper - wraps syscall to accept function pointer directly
// This overload allows passing function pointers without explicit casting
// Note: The generated multicore_launch_core1(uintptr_t) is included via app_syscalls.h
// We use a helper to avoid infinite recursion in the overload
namespace multicore_helpers {
  inline void launch_core1_impl(uintptr_t entry_addr) {
    // Call the generated syscall wrapper
    multicore_launch_core1(entry_addr);
  }
}

inline void multicore_launch_core1(void (*entry)(void)) {
  multicore_helpers::launch_core1_impl(reinterpret_cast<uintptr_t>(entry));
}

// BOOTSEL and softwareReset are available as syscalls
// They're included via app_syscalls.h, so no wrapper needed
// Just use BOOTSEL() and softwareReset() directly in your code

// Interrupt support - wraps syscalls to accept function pointers directly
// Note: The syscall wrappers (attachInterrupt/detachInterrupt with uintptr_t) 
// are generated in app_syscalls.h. We provide an overload for attachInterrupt that accepts function pointers.
// detachInterrupt doesn't need an overload since it only takes a pin number.
// __syscall_raw is already declared in app_syscalls.h, so we can use it directly.
#include "syscall_ids.h"

// Overload attachInterrupt to accept function pointer (the generated one takes uintptr_t)
inline void attachInterrupt(uint8_t pin, void (*isr)(), int mode) {
  // Call the generated syscall wrapper via syscall ID to avoid recursion
  // __syscall_raw is already declared in app_syscalls.h (included above)
  // __syscall_raw takes: uint16_t id + 30 uintptr_t args (a0 through a29)
  // We pass: pin (a0), isr (a1), mode (a2), then 27 zeros (a3-a29) = 30 total uintptr_t
  #include "syscall_ids.h"
  __syscall_raw(SYSC_attachInterrupt, 
    static_cast<uintptr_t>(pin),           // a0
    reinterpret_cast<uintptr_t>(isr),       // a1
    static_cast<uintptr_t>(mode),          // a2
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         // a3-a12 (10 zeros)
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,         // a13-a22 (10 zeros)
    0, 0, 0, 0, 0, 0, 0                   // a23-a29 (7 zeros)
  );  // Total: 3 + 10 + 10 + 7 = 30 uintptr_t arguments
}
// Note: detachInterrupt(uint8_t) is already generated in app_syscalls.h, so we don't need to wrap it

// WiFi proxy - forwards calls to syscall wrappers
struct WiFiProxy {
  inline int begin(const char* ssid, const char* password) { return WiFi_begin(ssid, password); }
  inline int begin(const char* ssid) { return WiFi_begin_open(ssid); }
  inline int status() { return WiFi_status(); }
  inline int disconnect() { return WiFi_disconnect(); }
  inline uint32_t localIP() { return WiFi_localIP(); }
  inline uint32_t subnetMask() { return WiFi_subnetMask(); }
  inline uint32_t gatewayIP() { return WiFi_gatewayIP(); }
  inline const char* SSID() { return WiFi_SSID(); }
  inline int32_t RSSI() { return WiFi_RSSI(); }
  inline uint8_t* macAddress(uint8_t* mac) { return WiFi_macAddress(mac); }
  inline int hostname(const char* hostname) { return WiFi_hostname(hostname); }
  inline const char* getHostname() { return WiFi_getHostname(); }
};

// Global WiFi object for app code
inline WiFiProxy WiFi;

// BLE proxy - forwards calls to syscall wrappers
struct BLEProxy {
  inline bool begin() { return BLE_begin(); }
  inline void end() { return BLE_end(); }
  inline void advertise() { return BLE_advertise(); }
  inline void stopAdvertise() { return BLE_stopAdvertise(); }
  inline void setAdvertisedServiceData(uint16_t uuid, const uint8_t* data, uint8_t length) {
    return BLE_setAdvertisedServiceData(uuid, data, length);
  }
  inline void setAdvertisedServiceUuid(const char* uuid) {
    return BLE_setAdvertisedServiceUuid(uuid);
  }
  inline void setLocalName(const char* name) { return BLE_setLocalName(name); }
  inline bool available() { return BLE_available(); }
  inline bool central() { return BLE_central(); }
  inline bool connected() { return BLE_connected(); }
  inline void disconnect() { return BLE_disconnect(); }
  inline const char* address() { return BLE_address(); }
};

// Global BLE object for app code
inline BLEProxy BLE;

#endif