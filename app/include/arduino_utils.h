#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Arduino utility function declarations
// These are not always available in standard C library

char* itoa(int value, char* str, int base);
char* utoa(unsigned int value, char* str, int base);
char* ltoa(long value, char* str, int base);
char* ultoa(unsigned long value, char* str, int base);
char* dtostrf(double val, signed char width, unsigned char prec, char *sout);

#ifdef __cplusplus
}
#endif

