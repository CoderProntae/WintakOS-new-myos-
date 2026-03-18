/*============================================================================
 * WintakOS - ports.h
 * x86 I/O Port Erişim Fonksiyonları
 *
 * x86 mimarisinde donanım cihazlarıyla iletişim iki yolla yapılır:
 *   1. Memory-Mapped I/O (MMIO) — bellek adresleri üzerinden
 *   2. Port-Mapped I/O (PMIO) — IN/OUT komutları ile
 *
 * PIC, PIT, klavye gibi geleneksel cihazlar port I/O kullanır.
 * Bu dosya inline assembly ile IN/OUT komutlarını sağlar.
 *==========================================================================*/

#ifndef WINTAKOS_PORTS_H
#define WINTAKOS_PORTS_H

#include "types.h"

/*
 * Bir porttan 8-bit veri oku.
 *
 * IN komutu: AL ← port[DX]
 * "=a" : sonuç AL/AX/EAX registerında döner
 * "Nd" : port numarası DX registerına veya immediate değere gider
 */
static inline uint8_t inb(uint16_t port)
{
    uint8_t result;
    __asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/*
 * Bir porta 8-bit veri yaz.
 *
 * OUT komutu: port[DX] ← AL
 */
static inline void outb(uint16_t port, uint8_t data)
{
    __asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}

/*
 * Bir porttan 16-bit veri oku.
 */
static inline uint16_t inw(uint16_t port)
{
    uint16_t result;
    __asm__ volatile ("inw %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

/*
 * Bir porta 16-bit veri yaz.
 */
static inline void outw(uint16_t port, uint16_t data)
{
    __asm__ volatile ("outw %0, %1" : : "a"(data), "Nd"(port));
}

/*
 * Kısa I/O gecikmesi (~1-4 mikrosaniye).
 *
 * Bazı eski donanımlar (özellikle PIC) ardışık port yazma işlemleri
 * arasında kısa bir gecikme gerektirir. Port 0x80 kullanılmayan
 * POST diagnostic portudur, yazma işlemi güvenlidir.
 */
static inline void io_wait(void)
{
    outb(0x80, 0);
}

#endif /* WINTAKOS_PORTS_H */
