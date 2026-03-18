/*============================================================================
 * WintakOS - vga_font.c
 * VGA Text Mode Font Modifikasyonu
 *
 * VGA fontu plane 2'de 0xA0000 adresinde saklanir.
 * Her karakter 32 byte yer kaplar (8x16 font icin ilk 16 byte kullanilir).
 * Sequencer ve Graphics Controller register'lari ile plane 2'ye erisim
 * saglanir, glif yazilir, sonra text mode'a geri donulur.
 *==========================================================================*/

#include "vga_font.h"
#include "../cpu/ports.h"

/* 8x16 piksel Turkce karakter glifleri */

/* ı - noktasiz kucuk i */
static const uint8_t glyph_dotless_i[16] = {
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x38,  /*   ***    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x3C,  /*   ****   */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
};

/* İ - noktali buyuk I */
static const uint8_t glyph_dotted_I[16] = {
    0x18,  /*    **    */
    0x00,  /*          */
    0x3C,  /*   ****   */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x18,  /*    **    */
    0x3C,  /*   ****   */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
};

/* ş - sedilli kucuk s */
static const uint8_t glyph_s_cedilla[16] = {
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x3C,  /*   ****   */
    0x66,  /*  **  **  */
    0x60,  /*  **      */
    0x3C,  /*   ****   */
    0x06,  /*      **  */
    0x66,  /*  **  **  */
    0x3C,  /*   ****   */
    0x08,  /*     *    */
    0x10,  /*    *     */
    0x00,  /*          */
    0x00,  /*          */
};

/* Ş - sedilli buyuk S */
static const uint8_t glyph_S_cedilla[16] = {
    0x00,  /*          */
    0x00,  /*          */
    0x3C,  /*   ****   */
    0x66,  /*  **  **  */
    0x60,  /*  **      */
    0x30,  /*   **     */
    0x18,  /*    **    */
    0x0C,  /*     **   */
    0x06,  /*      **  */
    0x66,  /*  **  **  */
    0x66,  /*  **  **  */
    0x3C,  /*   ****   */
    0x08,  /*     *    */
    0x10,  /*    *     */
    0x00,  /*          */
    0x00,  /*          */
};

/* ğ - yukmusak g (breve isareti) */
static const uint8_t glyph_g_breve[16] = {
    0x00,  /*          */
    0x24,  /*   *  *   */
    0x18,  /*    **    */
    0x00,  /*          */
    0x00,  /*          */
    0x3E,  /*   *****  */
    0x66,  /*  **  **  */
    0x66,  /*  **  **  */
    0x66,  /*  **  **  */
    0x3E,  /*   *****  */
    0x06,  /*      **  */
    0x66,  /*  **  **  */
    0x3C,  /*   ****   */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
};

/* Ğ - buyuk yukmusak G (breve isareti) */
static const uint8_t glyph_G_breve[16] = {
    0x24,  /*   *  *   */
    0x18,  /*    **    */
    0x3C,  /*   ****   */
    0x66,  /*  **  **  */
    0xC0,  /* **       */
    0xC0,  /* **       */
    0xC0,  /* **       */
    0xCE,  /* **  ***  */
    0xC6,  /* **   **  */
    0xC6,  /* **   **  */
    0x66,  /*  **  **  */
    0x3C,  /*   ****   */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
    0x00,  /*          */
};

/*
 * Tek bir karakterin glifini VGA font bellegine yazar.
 *
 * VGA font bellegi plane 2'dedir (0xA0000).
 * Her karakter 32 byte kaplar, sadece ilk 16'si kullanilir (8x16).
 */
static void vga_font_set_glyph(uint8_t index, const uint8_t bitmap[16])
{
    volatile uint8_t* font_mem = (volatile uint8_t*)0xA0000;

    /* --- Plane 2'ye yazma erisimi ac --- */

    /* Sequencer: Map Mask → sadece plane 2 */
    outb(0x3C4, 0x02);
    outb(0x3C5, 0x04);

    /* Sequencer: Memory Mode → sequential access, odd/even kapali */
    outb(0x3C4, 0x04);
    outb(0x3C5, 0x06);

    /* Graphics Controller: Read Map Select → plane 2 */
    outb(0x3CE, 0x04);
    outb(0x3CF, 0x02);

    /* Graphics Controller: Graphics Mode → duz yazma, interleave yok */
    outb(0x3CE, 0x05);
    outb(0x3CF, 0x00);

    /* Graphics Controller: Misc → 0xA0000 adresi, 64KB pencere */
    outb(0x3CE, 0x06);
    outb(0x3CF, 0x0C);

    /* --- Glif verisini yaz --- */
    uint32_t offset = (uint32_t)index * 32;
    for (int i = 0; i < 16; i++) {
        font_mem[offset + i] = bitmap[i];
    }

    /* --- Text mode'a geri don --- */

    /* Sequencer: Map Mask → plane 0 ve 1 */
    outb(0x3C4, 0x02);
    outb(0x3C5, 0x03);

    /* Sequencer: Memory Mode → odd/even aktif */
    outb(0x3C4, 0x04);
    outb(0x3C5, 0x02);

    /* Graphics Controller: Read Map Select → plane 0 */
    outb(0x3CE, 0x04);
    outb(0x3CF, 0x00);

    /* Graphics Controller: Graphics Mode → odd/even */
    outb(0x3CE, 0x05);
    outb(0x3CF, 0x10);

    /* Graphics Controller: Misc → 0xB8000, text mode */
    outb(0x3CE, 0x06);
    outb(0x3CF, 0x0E);
}

void vga_font_install_turkish(void)
{
    vga_font_set_glyph(CHAR_TR_DOTLESS_I,     glyph_dotless_i);
    vga_font_set_glyph(CHAR_TR_DOTTED_I,      glyph_dotted_I);
    vga_font_set_glyph(CHAR_TR_S_CEDILLA,     glyph_s_cedilla);
    vga_font_set_glyph(CHAR_TR_S_CEDILLA_UP,  glyph_S_cedilla);
    vga_font_set_glyph(CHAR_TR_G_BREVE,       glyph_g_breve);
    vga_font_set_glyph(CHAR_TR_G_BREVE_UP,    glyph_G_breve);
}
