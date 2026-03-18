#ifndef WINTAKOS_VGA_FONT_H
#define WINTAKOS_VGA_FONT_H

#include "../include/types.h"

/* Turkce ozel karakterler icin CP437 kod noktalari
 * 0x01-0x06 pozisyonlarini kullaniyoruz (normalde smiley/kart simgeleri) */
#define CHAR_TR_DOTLESS_I     0x01   /* ı */
#define CHAR_TR_DOTTED_I      0x02   /* İ */
#define CHAR_TR_S_CEDILLA     0x03   /* ş */
#define CHAR_TR_S_CEDILLA_UP  0x04   /* Ş */
#define CHAR_TR_G_BREVE       0x05   /* ğ */
#define CHAR_TR_G_BREVE_UP    0x06   /* Ğ */

void vga_font_install_turkish(void);

#endif
