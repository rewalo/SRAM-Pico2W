// Arduino utility functions - itoa, utoa, ltoa, ultoa, dtostrf
// These are not always available in standard C library

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

// Convert unsigned integer to string with base
static void reverse(char* str, int len) {
    int i = 0, j = len - 1;
    while (i < j) {
        char temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++;
        j--;
    }
}

char* itoa(int value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    int i = 0;
    int isNegative = 0;
    
    // Handle 0 explicitly
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    // Handle negative numbers only for base 10
    if (value < 0 && base == 10) {
        isNegative = 1;
        value = -value;
    }
    
    // Process individual digits
    while (value != 0) {
        int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }
    
    // Append negative sign for base 10
    if (isNegative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    reverse(str, i);
    return str;
}

char* utoa(unsigned int value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    int i = 0;
    
    // Handle 0 explicitly
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    // Process individual digits
    while (value != 0) {
        unsigned int rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }
    
    str[i] = '\0';
    reverse(str, i);
    return str;
}

char* ltoa(long value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    int i = 0;
    int isNegative = 0;
    
    // Handle 0 explicitly
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    // Handle negative numbers only for base 10
    if (value < 0 && base == 10) {
        isNegative = 1;
        value = -value;
    }
    
    // Process individual digits
    while (value != 0) {
        long rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }
    
    // Append negative sign for base 10
    if (isNegative) {
        str[i++] = '-';
    }
    
    str[i] = '\0';
    reverse(str, i);
    return str;
}

char* ultoa(unsigned long value, char* str, int base) {
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    int i = 0;
    
    // Handle 0 explicitly
    if (value == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    // Process individual digits
    while (value != 0) {
        unsigned long rem = value % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        value = value / base;
    }
    
    str[i] = '\0';
    reverse(str, i);
    return str;
}

char* dtostrf(double val, signed char width, unsigned char prec, char *sout) {
    char fmt[20];
    sprintf(fmt, "%%%d.%df", width, prec);
    sprintf(sout, fmt, val);
    return sout;
}

#ifdef __cplusplus
}
#endif

