#ifndef WINTAKOS_FBCONSOLE_H
#define WINTAKOS_FBCONSOLE_H

#include "../include/types.h"

/*============================================================================
 * Framebuffer Console
 * Grafik modu uzerinde text konsol emulasyonu
 *==========================================================================*/

#define FBCON_CHAR_W   8
#define FBCON_CHAR_H   16

void fbcon_init(void);
void fbcon_clear(void);
void fbcon_set_color(uint32_t fg, uint32_t bg);
void fbcon_putchar(uint8_t c);
void fbcon_puts(const char* str);
void fbcon_put_hex(uint32_t value);
void fbcon_put_dec(uint32_t value);
void fbcon_set_cursor(uint32_t row, uint32_t col);
uint32_t fbcon_get_row(void);
uint32_t fbcon_get_col(void);

#endif
