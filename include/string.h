/*============================================================================
 * WintakOS - string.h
 * Temel Bellek/String İşlem Fonksiyonları
 *
 * GCC freestanding modda implicit olarak memset/memcpy çağrıları
 * üretebilir (yapı atamaları, dizi kopyalama vb.). Bu yüzden
 * bu fonksiyonların linklenebilir tanımları olmalıdır.
 *==========================================================================*/

#ifndef WINTAKOS_STRING_H
#define WINTAKOS_STRING_H

#include "types.h"

/* Bellek bloğunu belirtilen değerle doldur */
void* memset(void* dest, int val, size_t count);

/* Bellek bloğunu kaynak adresinden kopyala */
void* memcpy(void* dest, const void* src, size_t count);

/* İki bellek bloğunu karşılaştır */
int memcmp(const void* a, const void* b, size_t count);

/* String uzunluğunu döndür (null karakter hariç) */
size_t strlen(const char* str);

#endif /* WINTAKOS_STRING_H */
