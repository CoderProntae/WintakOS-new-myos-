/*============================================================================
 * WintakOS - pit.h
 * Programmable Interval Timer (8253/8254 PIT) - Başlık
 *
 * PIT, belirli aralıklarla IRQ0 üreten donanım zamanlayıcısıdır.
 * 3 kanalı vardır, biz Channel 0'ı kullanıyoruz (sistem zamanlayıcı).
 *
 * Temel frekans: 1.193.180 Hz
 * Divisor ayarlanarak istenen frekansta kesme üretilir:
 *   Divisor = 1193180 / istenen_frekans
 *   Örnek: 100 Hz → Divisor = 11932 → her 10ms'de bir interrupt
 *==========================================================================*/

#ifndef WINTAKOS_PIT_H
#define WINTAKOS_PIT_H

#include "types.h"

/*--- Port Adresleri ---*/
#define PIT_CHANNEL0    0x40    /* Channel 0 data port */
#define PIT_CHANNEL1    0x41    /* Channel 1 data port */
#define PIT_CHANNEL2    0x42    /* Channel 2 data port */
#define PIT_COMMAND     0x43    /* Mode/Command register */

/*--- PIT Temel Frekansı ---*/
#define PIT_BASE_FREQ   1193180

/*--- Genel Fonksiyonlar ---*/

/* PIT'i belirtilen frekansta (Hz) başlat */
void pit_init(uint32_t frequency);

/* Toplam geçen tick sayısını döndür */
uint32_t pit_get_ticks(void);

/* Belirtilen tick sayısı kadar bekle (busy wait) */
void pit_sleep(uint32_t ticks);

#endif /* WINTAKOS_PIT_H */
