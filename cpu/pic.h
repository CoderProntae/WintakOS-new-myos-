/*============================================================================
 * WintakOS - pic.h
 * 8259A Programmable Interrupt Controller (PIC) - Başlık
 *
 * IBM PC'de iki kaskad PIC bulunur:
 *   Master PIC (PIC1): IRQ 0-7   → portlar 0x20 (command), 0x21 (data)
 *   Slave PIC  (PIC2): IRQ 8-15  → portlar 0xA0 (command), 0xA1 (data)
 *
 * Slave PIC, Master PIC'in IRQ2 hattına bağlıdır (cascade).
 *==========================================================================*/

#ifndef WINTAKOS_PIC_H
#define WINTAKOS_PIC_H

#include "types.h"

/*--- Port Adresleri ---*/
#define PIC1_COMMAND    0x20
#define PIC1_DATA       0x21
#define PIC2_COMMAND    0xA0
#define PIC2_DATA       0xA1

/*--- Komut Sabitleri ---*/
#define PIC_EOI         0x20    /* End of Interrupt komutu */

/*--- Remap Offset'leri ---*/
#define PIC1_OFFSET     32      /* Master: IRQ 0-7  → INT 32-39 */
#define PIC2_OFFSET     40      /* Slave:  IRQ 8-15 → INT 40-47 */

/*--- Genel Fonksiyonlar ---*/

/* PIC'leri başlat ve IRQ'ları remap et */
void pic_init(void);

/* End of Interrupt sinyali gönder (IRQ numarası: 0-15) */
void pic_send_eoi(uint8_t irq);

/* Belirli bir IRQ'yu aç (unmask) */
void pic_unmask(uint8_t irq);

/* Belirli bir IRQ'yu kapa (mask) */
void pic_mask(uint8_t irq);

#endif /* WINTAKOS_PIC_H */
