#ifndef WINTAKOS_VGA_H
#define WINTAKOS_VGA_H

#include "../include/types.h"

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define VGA_MEMORY  0xB8000

typedef enum {
    VGA_BLACK         = 0,  VGA_BLUE          = 1,
    VGA_GREEN         = 2,  VGA_CYAN          = 3,
    VGA_RED           = 4,  VGA_MAGENTA       = 5,
    VGA_BROWN         = 6,  VGA_LIGHT_GREY    = 7,
    VGA_DARK_GREY     = 8,  VGA_LIGHT_BLUE    = 9,
    VGA_LIGHT_GREEN   = 10, VGA_LIGHT_CYAN    = 11,
    VGA_LIGHT_RED     = 12, VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW        = 14, VGA_WHITE         = 15
} vga_color_t;

void    vga_init(void);
void    vga_clear(void);
void    vga_set_color(vga_color_t fg, vga_color_t bg);
void    vga_putchar(char c);
void    vga_puts(const char* str);
void    vga_put_hex(uint32_t value);
void    vga_put_dec(uint32_t value);
void    vga_set_cursor(uint8_t row, uint8_t col);
uint8_t vga_get_row(void);
uint8_t vga_get_col(void);

/* Milestone 2: Donanım imleci kontrolü */
void    vga_enable_cursor(uint8_t start, uint8_t end);
void    vga_disable_cursor(void);
void    vga_update_hw_cursor(void);

#endif
