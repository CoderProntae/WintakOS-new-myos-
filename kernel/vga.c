#include "vga.h"

static uint16_t* const  vga_buffer = (uint16_t*)VGA_MEMORY;
static uint8_t          vga_row    = 0;
static uint8_t          vga_col    = 0;
static uint8_t          vga_attr   = 0x0F;

static inline uint16_t vga_entry(uint8_t c, uint8_t attr)
{
    return (uint16_t)c | ((uint16_t)attr << 8);
}

static inline uint8_t vga_make_color(vga_color_t fg, vga_color_t bg)
{
    return (uint8_t)fg | ((uint8_t)bg << 4);
}

static void vga_scroll(void)
{
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }
    for (int i = 0; i < VGA_WIDTH; i++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = vga_entry(' ', vga_attr);
    }
    vga_row = VGA_HEIGHT - 1;
}

void vga_init(void)
{
    vga_attr = vga_make_color(VGA_WHITE, VGA_BLACK);
    vga_row  = 0;
    vga_col  = 0;
    vga_clear();
}

void vga_clear(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', vga_attr);
    }
    vga_row = 0;
    vga_col = 0;
}

void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    vga_attr = vga_make_color(fg, bg);
}

void vga_set_cursor(uint8_t row, uint8_t col)
{
    if (row < VGA_HEIGHT) vga_row = row;
    if (col < VGA_WIDTH)  vga_col = col;
}

void vga_putchar(uint8_t c)
{
    switch (c) {
        case '\n':
            vga_col = 0;
            vga_row++;
            break;
        case '\r':
            vga_col = 0;
            break;
        case '\t':
            vga_col = (vga_col + 8) & ~7;
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
            break;
        case '\b':
            if (vga_col > 0) {
                vga_col--;
                vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', vga_attr);
            }
            break;
        default:
            vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_attr);
            vga_col++;
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
            break;
    }

    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_puts(const char* str)
{
    if (str == NULL) return;
    while (*str) {
        vga_putchar((uint8_t)*str);
        str++;
    }
}

void vga_put_hex(uint32_t value)
{
    static const char hex_digits[] = "0123456789ABCDEF";
    vga_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        vga_putchar(hex_digits[(value >> i) & 0x0F]);
    }
}

void vga_put_dec(uint32_t value)
{
    if (value == 0) {
        vga_putchar('0');
        return;
    }
    char buf[12];
    int pos = 0;
    while (value > 0) {
        buf[pos++] = '0' + (value % 10);
        value /= 10;
    }
    for (int i = pos - 1; i >= 0; i--) {
        vga_putchar((uint8_t)buf[i]);
    }
}

uint8_t vga_get_row(void) { return vga_row; }
uint8_t vga_get_col(void) { return vga_col; }
