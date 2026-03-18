/*============================================================================
 * WintakOS - vga.h
 * VGA Text Mode Sürücüsü - Başlık Dosyası
 *
 * VGA text mode, 80x25 karakter boyutunda bir ekran sağlar.
 * Her karakter 2 byte ile temsil edilir:
 *   Byte 0: ASCII karakter kodu
 *   Byte 1: Renk özniteliği (4 bit arka plan + 4 bit ön plan)
 *
 * Video belleği 0xB8000 adresinden başlar.
 *==========================================================================*/

#ifndef WINTAKOS_VGA_H
#define WINTAKOS_VGA_H

#include "types.h"

/*--- Ekran Boyutları ---*/
#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

/*--- VGA Renk Kodları (4-bit) ---*/
typedef enum {
    VGA_BLACK           = 0,
    VGA_BLUE            = 1,
    VGA_GREEN           = 2,
    VGA_CYAN            = 3,
    VGA_RED             = 4,
    VGA_MAGENTA         = 5,
    VGA_BROWN           = 6,
    VGA_LIGHT_GREY      = 7,
    VGA_DARK_GREY       = 8,
    VGA_LIGHT_BLUE      = 9,
    VGA_LIGHT_GREEN     = 10,
    VGA_LIGHT_CYAN      = 11,
    VGA_LIGHT_RED       = 12,
    VGA_LIGHT_MAGENTA   = 13,
    VGA_YELLOW          = 14,
    VGA_WHITE           = 15
} vga_color_t;

/*--- Genel Fonksiyonlar ---*/

/* VGA sürücüsünü başlat (ekranı temizler, imleci sıfırlar) */
void vga_init(void);

/* Ekranı mevcut renk ile temizle */
void vga_clear(void);

/* Yazı rengini ayarla (ön plan + arka plan) */
void vga_set_color(vga_color_t foreground, vga_color_t background);

/* Tek bir karakter yaz */
void vga_putchar(char c);

/* Bir string (null-terminated) yaz */
void vga_puts(const char* str);

/* 32-bit değeri hexadecimal olarak yaz (debug için) */
void vga_put_hex(uint32_t value);

/* 32-bit değeri ondalık (decimal) olarak yaz */
void vga_put_dec(uint32_t value);

/* İmleci belirtilen konuma taşı */
void vga_set_cursor(uint8_t row, uint8_t col);

#endif /* WINTAKOS_VGA_H */
