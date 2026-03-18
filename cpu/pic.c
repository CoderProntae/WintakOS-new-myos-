/*============================================================================
 * WintakOS - pic.c
 * 8259A Programmable Interrupt Controller (PIC) - Uygulama
 *
 * Başlatma Sırası (ICW = Initialization Command Word):
 *   ICW1 → ICW2 → ICW3 → ICW4
 *
 * Her ICW belirli bir port'a yazılır ve PIC'i yapılandırır.
 *==========================================================================*/

#include "pic.h"
#include "ports.h"

/*==========================================================================
 * PIC'leri başlat ve IRQ'ları yeniden eşle (remap).
 *
 * Varsayılan eşleme:
 *   IRQ 0-7  → INT 0-7   (CPU exception'ları ile çakışır!)
 *   IRQ 8-15 → INT 8-15  (CPU exception'ları ile çakışır!)
 *
 * Yeni eşleme:
 *   IRQ 0-7  → INT 32-39
 *   IRQ 8-15 → INT 40-47
 *
 * Başlatma sonrasında tüm IRQ'lar maskeli (kapalı) olur.
 * pit_init(), keyboard_init() gibi sürücüler kendi IRQ'larını açar.
 *========================================================================*/
void pic_init(void)
{
    /* Mevcut maskeleri kaydet */
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    (void)mask1;
    (void)mask2;

    /*--- ICW1: Başlatma başlasın ---*/
    /* Bit 4: Initialization mode
     * Bit 0: ICW4 gerekli
     * = 0x11 */
    outb(PIC1_COMMAND, 0x11);
    io_wait();
    outb(PIC2_COMMAND, 0x11);
    io_wait();

    /*--- ICW2: Vektör offset ---*/
    /* Master PIC: IRQ 0 = INT 32 */
    outb(PIC1_DATA, PIC1_OFFSET);
    io_wait();
    /* Slave PIC: IRQ 8 = INT 40 */
    outb(PIC2_DATA, PIC2_OFFSET);
    io_wait();

    /*--- ICW3: Kaskad yapılandırması ---*/
    /* Master: Slave PIC, IRQ2 hattında (bit 2 = 0x04) */
    outb(PIC1_DATA, 0x04);
    io_wait();
    /* Slave: Kaskad kimliği = 2 */
    outb(PIC2_DATA, 0x02);
    io_wait();

    /*--- ICW4: Çalışma modu ---*/
    /* Bit 0: 8086/88 mode (AEOI yerine manuel EOI)
     * = 0x01 */
    outb(PIC1_DATA, 0x01);
    io_wait();
    outb(PIC2_DATA, 0x01);
    io_wait();

    /*--- Tüm IRQ'ları maskele (kapat) ---*/
    /* Her bit bir IRQ'yu temsil eder. 1=maskeli, 0=açık */
    /* Sadece IRQ2 (cascade) açık bırakılmalı yoksa slave çalışmaz */
    outb(PIC1_DATA, 0xFB);     /* 11111011b: sadece IRQ2 açık */
    outb(PIC2_DATA, 0xFF);     /* 11111111b: tümü kapalı */
}

/*==========================================================================
 * End of Interrupt sinyali gönder.
 *
 * Her IRQ handler'ı tamamlandığında PIC'e EOI gönderilmeli.
 * Slave PIC üzerindeki IRQ'lar (8-15) için hem slave'e hem master'a
 * EOI gönderilir (çünkü slave, master'ın IRQ2'sine bağlıdır).
 *========================================================================*/
void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8) {
        outb(PIC2_COMMAND, PIC_EOI);    /* Slave PIC'e EOI */
    }
    outb(PIC1_COMMAND, PIC_EOI);        /* Master PIC'e EOI */
}

/*==========================================================================
 * Belirli bir IRQ'yu aç (unmask).
 *
 * IRQ 0-7: Master PIC data portundaki ilgili bit temizlenir
 * IRQ 8-15: Slave PIC data portundaki ilgili bit temizlenir
 *========================================================================*/
void pic_unmask(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) & ~(1 << irq);   /* İlgili bit'i temizle (0=açık) */
    outb(port, value);
}

/*==========================================================================
 * Belirli bir IRQ'yu kapat (mask).
 *========================================================================*/
void pic_mask(uint8_t irq)
{
    uint16_t port;
    uint8_t  value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port) | (1 << irq);    /* İlgili bit'i set et (1=maskeli) */
    outb(port, value);
}
