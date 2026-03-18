#ifndef WINTAKOS_KEYMAP_TR_H
#define WINTAKOS_KEYMAP_TR_H

/*============================================================================
 * Turkce Q Klavye Haritasi
 *
 * PS/2 Scan Code Set 1 -> ASCII/karakter donusumu
 * Index = scancode (0x00 - 0x58)
 *
 * VGA text mode sadece 8-bit karakter destekler.
 * Turkce ozel karakterler (ç,ş,ğ,ü,ö,ı,İ) icin:
 *   - Code Page 857 (DOS Turkish) kodlarini kullaniyoruz
 *   - VESA grafik moduna gectigimizde UTF-8 destegi eklenecek
 *
 * CP857 kodlari:
 *   ç = 0x87,  Ç = 0x80
 *   ş = 0x9F,  Ş = 0x9E  (non-standard, font bagimlı)
 *   ğ = 0xA7,  Ğ = 0xA6  (non-standard)
 *   ü = 0x81,  Ü = 0x9A
 *   ö = 0x94,  Ö = 0x99
 *   ı = 0x8D,  İ = 0x98
 *
 * NOT: Bu kodlar standart VGA fontuyla goruntulenemez.
 *      Simdilik ASCII yakınlarını kullanıyoruz.
 *      Phase 5'te ozel font + UTF-8 ile degistirilecek.
 *==========================================================================*/

/* Normal (shift yok) */
static const char scancode_to_ascii[128] = {
/*          0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
/* 0x00 */  0,    0,   '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '*', '-',  0,    0,
/* 0x10 */ 'q', 'w', 'e', 'r', 't', 'y', 'u',  0,  'o', 'p',  0,   0,    0,    0,  'a', 's',
/* 0x20 */ 'd', 'f', 'g', 'h', 'j', 'k', 'l',  0,   0,  '"',  0,  ',', 'z', 'x', 'c', 'v',
/* 0x30 */ 'b', 'n', 'm',  0,   0,  '.',  0,  '*',  0,  ' ',  0,    0,    0,    0,    0,    0,
/* 0x40 */  0,    0,    0,    0,    0,    0,    0,  '7', '8', '9', '-', '4', '5', '6', '+', '1',
/* 0x50 */ '2', '3', '0', '.',  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};

/* Shift basili */
static const char scancode_to_ascii_shift[128] = {
/*          0     1     2     3     4     5     6     7     8     9     A     B     C     D     E     F  */
/* 0x00 */  0,    0,   '!', '\'', '^', '+', '%', '&', '/', '(', ')', '=', '?', '_',  0,    0,
/* 0x10 */ 'Q', 'W', 'E', 'R', 'T', 'Y', 'U',  0,  'O', 'P',  0,   0,    0,    0,  'A', 'S',
/* 0x20 */ 'D', 'F', 'G', 'H', 'J', 'K', 'L',  0,   0,   0,   0,  ';', 'Z', 'X', 'C', 'V',
/* 0x30 */ 'B', 'N', 'M',  0,   0,  ':',  0,  '*',  0,  ' ',  0,    0,    0,    0,    0,    0,
/* 0x40 */  0,    0,    0,    0,    0,    0,    0,  '7', '8', '9', '-', '4', '5', '6', '+', '1',
/* 0x50 */ '2', '3', '0', '.',  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */  0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};

#endif
