#ifndef WINTAKOS_VGA_FONT_H
#define WINTAKOS_VGA_FONT_H

#include "../include/types.h"

/*============================================================================
 * Turkce Karakter Kodlari
 *
 * 0x08=BS, 0x09=TAB, 0x0A=LF, 0x0D=CR kullaniliyor,
 * bu yuzden o pozisyonlardan kaciniyoruz.
 *==========================================================================*/

#define CHAR_TR_DOTLESS_I       0x01   /* ı */
#define CHAR_TR_DOTTED_CAP_I    0x02   /* İ */
#define CHAR_TR_S_CEDILLA       0x03   /* ş */
#define CHAR_TR_S_CEDILLA_UP    0x04   /* Ş */
#define CHAR_TR_G_BREVE         0x05   /* ğ */
#define CHAR_TR_G_BREVE_UP      0x06   /* Ğ */
#define CHAR_TR_U_UMLAUT        0x07   /* ü */
#define CHAR_TR_U_UMLAUT_UP     0x0B   /* Ü */
#define CHAR_TR_O_UMLAUT        0x0C   /* ö */
#define CHAR_TR_O_UMLAUT_UP     0x0E   /* Ö */
#define CHAR_TR_C_CEDILLA       0x0F   /* ç */
#define CHAR_TR_C_CEDILLA_UP    0x10   /* Ç */

void vga_font_install_turkish(void);

#endif
