/*============================================================================
 * WintakOS - pit.c
 * Programmable Interval Timer (PIT) - Uygulama
 *
 * Channel 0, Mode 3 (Square Wave Generator):
 *   - Sayıcı her PIT clock cycle'da 2 azalır
 *   - Sıfıra ulaşınca çıkış durumu değişir (toggle)
 *   - IRQ0 üretilir
 *
 * Command byte (0x36):
 *   Bit 7-6: 00 = Channel 0
 *   Bit 5-4: 11 = Access mode: lobyte/hibyte
 *   Bit 3-1: 011 = Mode 3 (Square Wave)
 *   Bit 0:   0   = 16-bit binary counter
 *==========================================================================*/

#include "pit.h"
#include "isr.h"
#include "pic.h"
#include "ports.h"

/*--- Tick sayacı (volatile: interrupt handler'da değiştirilir) ---*/
static volatile uint32_t pit_ticks = 0;

/*--- Yapılandırılmış frekans ---*/
static uint32_t pit_frequency = 0;

/*==========================================================================
 * PIT IRQ0 Handler
 *
 * Her timer tick'inde çağrılır. Tick sayacını artırır.
 * İleride: process scheduling, sleep wake-up, animasyon timer'ı
 *========================================================================*/
static void pit_handler(registers_t* regs)
{
    (void)regs;     /* Şu an register bilgisine ihtiyaç yok */
    pit_ticks++;
}

/*==========================================================================
 * PIT'i başlat.
 *
 * frequency: İstenen kesme frekansı (Hz).
 *            Önerilen: 100 (her 10ms) veya 1000 (her 1ms)
 *            100 Hz, kernel boot aşaması için yeterlidir.
 *========================================================================*/
void pit_init(uint32_t frequency)
{
    pit_frequency = frequency;
    pit_ticks = 0;

    /* Divisor hesapla */
    uint16_t divisor = (uint16_t)(PIT_BASE_FREQ / frequency);

    /* Command: Channel 0, lobyte/hibyte, Mode 3, binary */
    outb(PIT_COMMAND, 0x36);

    /* Divisor'ı iki parçada gönder (lobyte → hibyte sırası) */
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));          /* Low byte  */
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));   /* High byte */

    /* IRQ0 handler'ını kaydet */
    isr_register_handler(32, pit_handler);

    /* IRQ0'ı aç */
    pic_unmask(0);
}

/*==========================================================================
 * Toplam geçen tick sayısını döndür.
 *========================================================================*/
uint32_t pit_get_ticks(void)
{
    return pit_ticks;
}

/*==========================================================================
 * Belirtilen tick sayısı kadar bekle.
 *
 * HLT komutu: Bir sonraki interrupt'a kadar CPU'yu düşük güç moduna alır.
 * Bu, boş döngüden (busy-wait) çok daha verimlidir.
 *
 * Kullanım: pit_sleep(100) → 100 Hz'de 1 saniye bekler
 *========================================================================*/
void pit_sleep(uint32_t ticks_to_wait)
{
    uint32_t start = pit_ticks;
    while ((pit_ticks - start) < ticks_to_wait) {
        __asm__ volatile ("hlt");
    }
}
