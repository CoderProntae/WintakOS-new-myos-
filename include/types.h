/*============================================================================
 * WintakOS - types.h
 * Temel Veri Tipi Tanımlamaları
 *
 * Neden kendi tiplerımızı tanımlıyoruz?
 * Freestanding ortamda (işletim sistemi yok) standart kütüphane yoktur.
 * GCC, freestanding modda <stdint.h> ve <stddef.h> sağlar, ancak
 * tam kontrol ve taşınabilirlik için kendi tiplerımızı tanımlıyoruz.
 * İleride ihtiyaç duyulursa GCC'nin freestanding başlıklarına geçilebilir.
 *==========================================================================*/

#ifndef WINTAKOS_TYPES_H
#define WINTAKOS_TYPES_H

/*--- İşaretsiz (unsigned) tam sayı tipleri ---*/
typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

/*--- İşaretli (signed) tam sayı tipleri ---*/
typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

/*--- Boyut ve adres tipleri ---*/
typedef uint32_t            size_t;
typedef int32_t             ssize_t;
typedef uint32_t            uintptr_t;
typedef int32_t             intptr_t;

/*--- Boolean tipi ---*/
typedef enum { false = 0, true = 1 } bool;

/*--- NULL tanımı ---*/
#define NULL ((void*)0)

#endif /* WINTAKOS_TYPES_H */
