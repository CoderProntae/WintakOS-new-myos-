#include "fbconsole.h"
#include "framebuffer.h"
#include "font8x16.h"
#include "../lib/string.h"

static uint32_t con_row = 0;
static uint32_t con_col = 0;
static uint32_t con_max_rows = 0;
static uint32_t con_max_cols = 0;
static uint32_t con_fg = COLOR_WHITE;
static uint32_t con_bg = COLOR_BG_DEFAULT;

void fbcon_init(void)
{
    framebuffer_t* fb = fb_get_info();
    if (!fb->available) return;

    con_max_cols = fb->width / FBCON_CHAR_W;
    con_max_rows = fb->height / FBCON_CHAR_H;
    con_row = 0;
    con_col = 0;
    con_fg = COLOR_WHITE;
    con_bg = COLOR_BG_DEFAULT;

    fb_clear(con_bg);
}

void fbcon_clear(void)
{
    fb_clear(con_bg);
    con_row = 0;
    con_col = 0;
}

void fbcon_set_color(uint32_t fg, uint32_t bg)
{
    con_fg = fg;
    con_bg = bg;
}

static void fbcon_draw_char(uint32_t row, uint32_t col, uint8_t c, uint32_t fg, uint32_t bg)
{
    uint32_t px = col * FBCON_CHAR_W;
    uint32_t py = row * FBCON_CHAR_H;

    const uint8_t* glyph;
    if (c <= 0x7F || c <= 0x10) {
        glyph = font8x16_data[c];
    } else {
        glyph = font8x16_data[0];
    }

    for (uint32_t y = 0; y < FBCON_CHAR_H; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < FBCON_CHAR_W; x++) {
            uint32_t color = (line & (0x80 >> x)) ? fg : bg;
            fb_put_pixel(px + x, py + y, color);
        }
    }
}

static void fbcon_scroll(void)
{
    framebuffer_t* fb = fb_get_info();
    if (!fb->available) return;

    uint32_t line_pixels = FBCON_CHAR_H;
    uint32_t pitch32 = fb->pitch / 4;

    /* Bir karakter yuksekligi kadar yukari kaydir */
    uint32_t copy_rows = fb->height - line_pixels;
    for (uint32_t row = 0; row < copy_rows; row++) {
        uint32_t dst = row * pitch32;
        uint32_t src = (row + line_pixels) * pitch32;
        memcpy(&fb->address[dst], &fb->address[src], fb->pitch);
    }

    /* Alt kismi temizle */
    for (uint32_t row = copy_rows; row < fb->height; row++) {
        uint32_t offset = row * pitch32;
        for (uint32_t col = 0; col < fb->width; col++) {
            fb->address[offset + col] = con_bg;
        }
    }

    con_row = con_max_rows - 1;
}

void fbcon_putchar(uint8_t c)
{
    switch (c) {
        case '\n':
            con_col = 0;
            con_row++;
            break;
        case '\r':
            con_col = 0;
            break;
        case '\t':
            con_col = (con_col + 8) & ~7;
            if (con_col >= con_max_cols) {
                con_col = 0;
                con_row++;
            }
            break;
        case '\b':
            if (con_col > 0) {
                con_col--;
                fbcon_draw_char(con_row, con_col, ' ', con_fg, con_bg);
            }
            break;
        default:
            fbcon_draw_char(con_row, con_col, c, con_fg, con_bg);
            con_col++;
            if (con_col >= con_max_cols) {
                con_col = 0;
                con_row++;
            }
            break;
    }

    if (con_row >= con_max_rows) {
        fbcon_scroll();
    }
}

void fbcon_puts(const char* str)
{
    if (!str) return;
    while (*str) {
        fbcon_putchar((uint8_t)*str);
        str++;
    }
}

void fbcon_put_hex(uint32_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    fbcon_puts("0x");
    for (int i = 28; i >= 0; i -= 4) {
        fbcon_putchar(hex[(value >> i) & 0x0F]);
    }
}

void fbcon_put_dec(uint32_t value)
{
    if (value == 0) { fbcon_putchar('0'); return; }
    char buf[12];
    int pos = 0;
    while (value > 0) {
        buf[pos++] = '0' + (value % 10);
        value /= 10;
    }
    for (int i = pos - 1; i >= 0; i--) {
        fbcon_putchar((uint8_t)buf[i]);
    }
}

void fbcon_set_cursor(uint32_t row, uint32_t col)
{
    if (row < con_max_rows) con_row = row;
    if (col < con_max_cols) con_col = col;
}

uint32_t fbcon_get_row(void) { return con_row; }
uint32_t fbcon_get_col(void) { return con_col; }
