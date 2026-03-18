/*============================================================================
 * WintakOS - gdt.h
 * Global Descriptor Table (GDT) - Başlık
 *
 * GDT, x86 Protected Mode'da bellek segmentlerini tanımlar.
 * Her giriş (entry) 8 byte'tır ve segmentin taban adresini,
 * limitini, erişim haklarını ve özelliklerini belirtir.
 *
 * Flat Model: Tüm segmentler 0x00000000 - 0xFFFFFFFF aralığını kapsar.
 * Bu sayede segmentasyon pratik olarak devre dışı kalır ve
 * tüm bellek yönetimi paging ile yapılabilir.
 *==========================================================================*/

#ifndef WINTAKOS_GDT_H
#define WINTAKOS_GDT_H

#include "types.h"

/*--- GDT Segment Sayısı ---*/
#define GDT_ENTRIES 5

/*--- Segment Seçici Sabitleri ---*/
#define GDT_KERNEL_CODE 0x08    /* Index 1, RPL 0 */
#define GDT_KERNEL_DATA 0x10    /* Index 2, RPL 0 */
#define GDT_USER_CODE   0x18    /* Index 3, RPL 3 */
#define GDT_USER_DATA   0x20    /* Index 4, RPL 3 */

/*--- GDT Giriş Yapısı (8 byte) ---*/
/*
 * Bellek düzeni (düşük adresten yükseğe):
 *
 *   Byte 0-1: Limit alt 16 bit
 *   Byte 2-3: Base alt 16 bit
 *   Byte 4:   Base orta 8 bit
 *   Byte 5:   Access byte
 *              Bit 7:   Present (1=geçerli segment)
 *              Bit 6-5: DPL (Descriptor Privilege Level, 0=kernel, 3=user)
 *              Bit 4:   Descriptor type (1=code/data, 0=system)
 *              Bit 3:   Executable (1=code, 0=data)
 *              Bit 2:   Direction/Conforming
 *              Bit 1:   Readable(code)/Writable(data)
 *              Bit 0:   Accessed
 *   Byte 6:   Granularity + Limit üst 4 bit
 *              Bit 7:   Granularity (1=4KB, 0=1 byte)
 *              Bit 6:   Size (1=32-bit, 0=16-bit)
 *              Bit 5:   Long mode (0 for 32-bit)
 *              Bit 4:   Available
 *              Bit 3-0: Limit üst 4 bit
 *   Byte 7:   Base üst 8 bit
 */
typedef struct {
    uint16_t limit_low;     /* Limit: alt 16 bit */
    uint16_t base_low;      /* Base:  alt 16 bit */
    uint8_t  base_middle;   /* Base:  orta 8 bit (bit 16-23) */
    uint8_t  access;        /* Erişim hakları */
    uint8_t  granularity;   /* Bayraklar + limit üst 4 bit */
    uint8_t  base_high;     /* Base:  üst 8 bit (bit 24-31) */
} __attribute__((packed)) gdt_entry_t;

/*--- GDT Pointer (LGDT komutu için) ---*/
typedef struct {
    uint16_t limit;         /* Tablonun toplam boyutu - 1 */
    uint32_t base;          /* Tablonun başlangıç adresi */
} __attribute__((packed)) gdt_ptr_t;

/*--- Genel Fonksiyonlar ---*/

/* GDT'yi başlat ve yükle */
void gdt_init(void);

#endif /* WINTAKOS_GDT_H */
