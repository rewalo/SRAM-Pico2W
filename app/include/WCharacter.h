#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <ctype.h>
#include <stdint.h>

// Character classification and conversion functions.
// These wrap libc's ctype.h functions for Arduino compatibility.

// Check if character is alphanumeric (letter or number).
inline uint8_t isAlphaNumeric(uint8_t c) {
  return static_cast<uint8_t>(isalnum(static_cast<int>(c)));
}

// Check if character is alphabetic (letter).
inline uint8_t isAlpha(uint8_t c) {
  return static_cast<uint8_t>(isalpha(static_cast<int>(c)));
}

// Check if character is ASCII (0-127).
inline uint8_t isAscii(uint8_t c) {
  return (c <= 127) ? 1 : 0;
}

// Check if character is a control character.
inline uint8_t isControl(uint8_t c) {
  return static_cast<uint8_t>(iscntrl(static_cast<int>(c)));
}

// Check if character is a digit (0-9).
inline uint8_t isDigit(uint8_t c) {
  return static_cast<uint8_t>(isdigit(static_cast<int>(c)));
}

// Check if character is printable (not a control character).
inline uint8_t isGraph(uint8_t c) {
  return static_cast<uint8_t>(isgraph(static_cast<int>(c)));
}

// Check if character is lowercase.
inline uint8_t isLowerCase(uint8_t c) {
  return static_cast<uint8_t>(islower(static_cast<int>(c)));
}

// Check if character is printable (including space).
inline uint8_t isPrintable(uint8_t c) {
  return static_cast<uint8_t>(isprint(static_cast<int>(c)));
}

// Check if character is punctuation.
inline uint8_t isPunct(uint8_t c) {
  return static_cast<uint8_t>(ispunct(static_cast<int>(c)));
}

// Check if character is whitespace (space, tab, newline, etc.).
inline uint8_t isSpace(uint8_t c) {
  return static_cast<uint8_t>(isspace(static_cast<int>(c)));
}

// Check if character is uppercase.
inline uint8_t isUpperCase(uint8_t c) {
  return static_cast<uint8_t>(isupper(static_cast<int>(c)));
}

// Check if character is a hexadecimal digit (0-9, A-F, a-f).
inline uint8_t isHexadecimalDigit(uint8_t c) {
  return static_cast<uint8_t>(isxdigit(static_cast<int>(c)));
}

// Character conversion functions.

// Convert character to lowercase.
inline uint8_t toLowerCase(uint8_t c) {
  return static_cast<uint8_t>(tolower(static_cast<int>(c)));
}

// Convert character to uppercase.
inline uint8_t toUpperCase(uint8_t c) {
  return static_cast<uint8_t>(toupper(static_cast<int>(c)));
}

#ifdef __cplusplus
}
#endif

