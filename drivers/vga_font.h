#ifndef WINTAKOS_VGA_FONT_H
#define WINTAKOS_VGA_FONT_H

#include "../include/types.h"

#define CHAR_TR_DOTLESS_I     0x01
#define CHAR_TR_DOTTED_I      0x02
#define CHAR_TR_S_CEDILLA     0x03
#define CHAR_TR_S_CEDILLA_UP  0x04
#define CHAR_TR_G_BREVE       0x05
#define CHAR_TR_G_BREVE_UP    0x06

void vga_font_install_turkish(void);

#endif
