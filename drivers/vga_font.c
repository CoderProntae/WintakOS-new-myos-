/*============================================================================
 * WintakOS - vga_font.c
 * VGA Text Mode Font - Turkce Karakter Glifleri
 *
 * Plane 2'ye erisim bir kez acilir, tum glifler yazilir,
 * sonra text mode'a geri donulur.
 *==========================================================================*/

#include "vga_font.h"
#include "../cpu/ports.h"

/* ı - noktasiz kucuk i */
static const uint8_t glyph_dotless_i[16] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x78, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x78,
    0x00, 0x00, 0x00, 0x00,
};

/* İ - noktali buyuk I */
static const uint8_t glyph_dotted_I[16] = {
    0x30, 0x30, 0x00, 0x78,
    0x30, 0x30, 0x30, 0x30,
    0x30, 0x30, 0x30, 0x78,
    0x00, 0x00, 0x00, 0x00,
};

/* ş - sedilli kucuk s */
static const uint8_t glyph_s_cedilla[16] = {
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x7C, 0xC0, 0xC0,
    0x7C, 0x06, 0x06, 0x7C,
    0x10, 0x20, 0x00, 0x00,
};

/* Ş - sedilli buyuk S */
static const uint8_t glyph_S_cedilla[16] = {
    0x00, 0x7C, 0xC6, 0xC0,
    0xC0, 0x7C, 0x06, 0x06,
    0x06, 0xC6, 0xC6, 0x7C,
    0x10, 0x20, 0x00, 0x00,
};

/* ğ - yukmusak g */
static const uint8_t glyph_g_breve[16] = {
    0x00, 0x44, 0x38, 0x00,
    0x00, 0x76, 0xCC, 0xCC,
    0xCC, 0x76, 0x0C, 0xCC,
    0x78, 0x00, 0x00, 0x00,
};

/* Ğ - buyuk yukmusak G */
static const uint8_t glyph_G_breve[16] = {
    0x44, 0x38, 0x00, 0x3C,
    0x66, 0xC0, 0xC0, 0xC0,
    0xCE, 0xC6, 0x66, 0x3C,
    0x00, 0x00, 0x00, 0x00,
};

/* Plane 2 yazma moduna gec */
static void font_begin_access(void)
{
    /* Sequencer register'larini kaydet ve ayarla */
    outb(0x3C4, 0x00); outb(0x3C5, 0x01);  /* Synchronous reset */
    outb(0x3C4, 0x02); outb(0x3C5, 0x04);  /* Map Mask: plane 2 */
    outb(0x3C4, 0x04); outb(0x3C5, 0x07);  /* Memory Mode: sequential, no odd/even, extended */
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);  /* Reset bitti */

    /* Graphics Controller register'larini ayarla */
    outb(0x3CE, 0x04); outb(0x3CF, 0x02);  /* Read Map Select: plane 2 */
    outb(0x3CE, 0x05); outb(0x3CF, 0x00);  /* Graphics Mode: write mode 0, no interleave */
    outb(0x3CE, 0x06); outb(0x3CF, 0x00);  /* Misc: map at 0xA0000, 256KB */
}

/* Text mode'a geri don */
static void font_end_access(void)
{
    outb(0x3C4, 0x00); outb(0x3C5, 0x01);  /* Synchronous reset */
    outb(0x3C4, 0x02); outb(0x3C5, 0x03);  /* Map Mask: plane 0+1 */
    outb(0x3C4, 0x04); outb(0x3C5, 0x03);  /* Memory Mode: odd/even */
    outb(0x3C4, 0x00); outb(0x3C5, 0x03);  /* Reset bitti */

    outb(0x3CE, 0x04); outb(0x3CF, 0x00);  /* Read Map Select: plane 0 */
    outb(0x3CE, 0x05); outb(0x3CF, 0x10);  /* Graphics Mode: odd/even interleave */
    outb(0x3CE, 0x06); outb(0x3CF, 0x0E);  /* Misc: map at 0xB8000, text mode */
}

void vga_font_install_turkish(void)
{
    volatile uint8_t* font_mem = (volatile uint8_t*)0xA0000;

    /* Bir kez plane 2'ye gec */
    font_begin_access();

    /* Her karakter 32 byte kaplar, sadece ilk 16 kullanilir */
    /* CHAR_TR_DOTLESS_I = 0x01 */
    uint32_t off;

    off = (uint32_t)CHAR_TR_DOTLESS_I * 32;
    for (int i = 0; i < 16; i++) font_mem[off + i] = glyph_dotless_i[i];

    off = (uint32_t)CHAR_TR_DOTTED_I * 32;
    for (int i = 0; i < 16; i++) font_mem[off + i] = glyph_dotted_I[i];

    off = (uint32_t)CHAR_TR_S_CEDILLA * 32;
    for (int i = 0; i < 16; i++) font_mem[off + i] = glyph_s_cedilla[i];

    off = (uint32_t)CHAR_TR_S_CEDILLA_UP * 32;
    for (int i = 0; i < 16; i++) font_mem[off + i] = glyph_S_cedilla[i];

    off = (uint32_t)CHAR_TR_G_BREVE * 32;
    for (int i = 0; i < 16; i++) font_mem[off + i] = glyph_g_breve[i];

    off = (uint32_t)CHAR_TR_G_BREVE_UP * 32;
    for (int i = 0; i < 16; i++) font_mem[off + i] = glyph_G_breve[i];

    /* Text mode'a geri don */
    font_end_access();
}
