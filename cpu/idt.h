/*============================================================================
 * WintakOS - idt.h
 * Interrupt Descriptor Table (IDT) - Başlık
 *
 * IDT, 256 interrupt vektörünü handler fonksiyonlarına eşler.
 *   0-31   : CPU Exception'ları (sıfıra bölme, page fault, vb.)
 *   32-47  : Donanım IRQ'ları (PIC tarafından yönlendirilir)
 *   48-255 : Yazılım interrupt'ları (kullanıcı tanımlı)
 *
 * Her IDT girişi 8 byte'tır ve handler'ın adresini, segment
 * seçicisini ve bayrakları içerir.
 *==========================================================================*/

#ifndef WINTAKOS_IDT_H
#define WINTAKOS_IDT_H

#include "types.h"

/*--- IDT Boyutu ---*/
#define IDT_ENTRIES 256

/*--- IDT Giriş Yapısı (8 byte) ---*/
/*
 *   Byte 0-1: Handler offset alt 16 bit
 *   Byte 2-3: Segment seçici (genelde 0x08 = kernel code)
 *   Byte 4:   Sıfır (rezerv)
 *   Byte 5:   Bayraklar (type + DPL + present)
 *              Bit 7:   Present
 *              Bit 6-5: DPL (0=kernel, 3=user)
 *              Bit 4:   Storage segment (0 for interrupt/trap)
 *              Bit 3-0: Gate type
 *                       0xE = 32-bit Interrupt Gate (IF otomatik temizlenir)
 *                       0xF = 32-bit Trap Gate (IF temizlenmez)
 *   Byte 6-7: Handler offset üst 16 bit
 */
typedef struct {
    uint16_t base_low;      /* Handler adresi: alt 16 bit */
    uint16_t selector;      /* Kod segment seçici */
    uint8_t  zero;          /* Her zaman 0 */
    uint8_t  flags;         /* Type, DPL, Present */
    uint16_t base_high;     /* Handler adresi: üst 16 bit */
} __attribute__((packed)) idt_entry_t;

/*--- IDT Pointer (LIDT komutu için) ---*/
typedef struct {
    uint16_t limit;         /* Tablonun toplam boyutu - 1 */
    uint32_t base;          /* Tablonun başlangıç adresi */
} __attribute__((packed)) idt_ptr_t;

/*--- IDT bayrak sabitleri ---*/
/* 0x8E = 10001110b: Present=1, DPL=0, Type=0xE (32-bit interrupt gate) */
#define IDT_FLAG_KERNEL_INT  0x8E

/* 0xEE = 11101110b: Present=1, DPL=3, Type=0xE (user-callable) */
#define IDT_FLAG_USER_INT    0xEE

/*--- Genel Fonksiyonlar ---*/
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);

#endif /* WINTAKOS_IDT_H */
