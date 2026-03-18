/*============================================================================
 * WintakOS - string.c
 * Temel Bellek/String İşlem Fonksiyonları - Uygulama
 *==========================================================================*/

#include "string.h"

void* memset(void* dest, int val, size_t count)
{
    uint8_t* d = (uint8_t*)dest;
    for (size_t i = 0; i < count; i++) {
        d[i] = (uint8_t)val;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, size_t count)
{
    uint8_t* d = (uint8_t*)dest;
    const uint8_t* s = (const uint8_t*)src;
    for (size_t i = 0; i < count; i++) {
        d[i] = s[i];
    }
    return dest;
}

int memcmp(const void* a, const void* b, size_t count)
{
    const uint8_t* p1 = (const uint8_t*)a;
    const uint8_t* p2 = (const uint8_t*)b;
    for (size_t i = 0; i < count; i++) {
        if (p1[i] != p2[i]) {
            return (int)p1[i] - (int)p2[i];
        }
    }
    return 0;
}

size_t strlen(const char* str)
{
    size_t len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}
