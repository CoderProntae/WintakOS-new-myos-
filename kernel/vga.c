/*============================================================================
 * WintakOS - vga.c
 * VGA Text Mode Sürücüsü - Uygulama
 *
 * Bu modül ekrana karakter ve string yazdırma, renk ayarlama,
 * satır kaydırma (scrolling) ve temel formatlı çıktı sağlar.
 *
 * Bellek düzeni (0xB8000):
 *   Her hücre 2 byte:
 *   [Byte 0: Karakter] [Byte 1: Öznitelik]
 *
 *   Öznitelik byte düzeni:
 *   Bit 7    : Blink (yanıp sönme, genelde devre dışı)
 *   Bit 6-4  : Arka plan rengi (3 bit = 8 renk)
 *   Bit 3-0  : Ön plan rengi  (4 bit = 16 renk)
 *==========================================================================*/

#include "vga.h"

/*--- Modül Durumu (static: sadece bu dosyadan erişilebilir) ---*/
static uint16_t* const  vga_buffer = (uint16_t*)VGA_MEMORY;
static uint8_t          vga_row    = 0;     /* Geçerli satır (0-24) */
static uint8_t          vga_col    = 0;     /* Geçerli sütun (0-79) */
static uint8_t          vga_attr   = 0x0F;  /* Varsayılan: beyaz on siyah */

/*==========================================================================
 * Yardımcı Fonksiyonlar (static inline)
 *========================================================================*/

/*
 * Karakter ve özniteliği tek bir 16-bit VGA girişine birleştirir.
 *   Alt 8 bit:  karakter kodu
 *   Üst 8 bit:  renk özniteliği
 */
static inline uint16_t vga_entry(char c, uint8_t attr)
{
    return (uint16_t)(unsigned char)c | ((uint16_t)attr << 8);
}

/*
 * Ön plan ve arka plan renklerini tek bir öznitelik byte'ına birleştirir.
 *   Alt 4 bit:  ön plan rengi
 *   Üst 4 bit:  arka plan rengi
 */
static inline uint8_t vga_make_color(vga_color_t fg, vga_color_t bg)
{
    return (uint8_t)fg | ((uint8_t)bg << 4);
}

/*==========================================================================
 * Dahili İşlevler
 *========================================================================*/

/*
 * Ekranı bir satır yukarı kaydırır.
 * En üst satır silinir, en alt satır boş kalır.
 * Bu fonksiyon imleç son satırın altına indiğinde çağrılır.
 */
static void vga_scroll(void)
{
    /* Satır 1'den itibaren tüm satırları bir üste kopyala */
    for (int i = 0; i < VGA_WIDTH * (VGA_HEIGHT - 1); i++) {
        vga_buffer[i] = vga_buffer[i + VGA_WIDTH];
    }

    /* Son satırı boşalt */
    for (int i = 0; i < VGA_WIDTH; i++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + i] = vga_entry(' ', vga_attr);
    }

    /* İmleç son satırda kalsın */
    vga_row = VGA_HEIGHT - 1;
}

/*==========================================================================
 * Genel (Public) Fonksiyonlar
 *========================================================================*/

/*
 * VGA sürücüsünü başlatır.
 * Varsayılan rengi ayarlar ve ekranı temizler.
 */
void vga_init(void)
{
    vga_attr = vga_make_color(VGA_WHITE, VGA_BLACK);
    vga_row  = 0;
    vga_col  = 0;
    vga_clear();
}

/*
 * Tüm ekranı mevcut öznitelik rengiyle temizler.
 * İmleci sol üst köşeye (0,0) sıfırlar.
 */
void vga_clear(void)
{
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = vga_entry(' ', vga_attr);
    }
    vga_row = 0;
    vga_col = 0;
}

/*
 * Aktif yazı rengini değiştirir.
 * Bu çağrıdan sonra yazılan tüm karakterler yeni renkte olur.
 */
void vga_set_color(vga_color_t fg, vga_color_t bg)
{
    vga_attr = vga_make_color(fg, bg);
}

/*
 * İmleci belirtilen satır ve sütuna konumlandırır.
 * Sınır kontrolü yapar.
 */
void vga_set_cursor(uint8_t row, uint8_t col)
{
    if (row < VGA_HEIGHT) vga_row = row;
    if (col < VGA_WIDTH)  vga_col = col;
}

/*
 * Tek bir ASCII karakter yazar.
 * Özel karakterleri işler: \n (yeni satır), \r (satır başı), \t (tab).
 * Satır sonuna gelindiğinde otomatik alt satıra geçer.
 * Ekranın sonuna gelindiğinde otomatik kaydırma yapar.
 */
void vga_putchar(char c)
{
    switch (c) {
        case '\n':
            /* Yeni satır: sütunu sıfırla, satırı artır */
            vga_col = 0;
            vga_row++;
            break;

        case '\r':
            /* Satır başı: sadece sütunu sıfırla */
            vga_col = 0;
            break;

        case '\t':
            /* Tab: 8'in katına yuvarla */
            vga_col = (vga_col + 8) & ~7;
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
            break;

        case '\b':
            /* Backspace: bir karakter geri git */
            if (vga_col > 0) {
                vga_col--;
                vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(' ', vga_attr);
            }
            break;

        default:
            /* Normal karakter: buffer'a yaz ve ilerle */
            vga_buffer[vga_row * VGA_WIDTH + vga_col] = vga_entry(c, vga_attr);
            vga_col++;

            /* Satır sonuna gelindi mi? */
            if (vga_col >= VGA_WIDTH) {
                vga_col = 0;
                vga_row++;
            }
            break;
    }

    /* Ekranın altına gelindi mi? Kaydır. */
    if (vga_row >= VGA_HEIGHT) {
        vga_scroll();
    }
}

/*
 * Null-terminated string yazar.
 * Her karakteri sırayla vga_putchar() ile işler.
 */
void vga_puts(const char* str)
{
    if (str == NULL) return;

    while (*str) {
        vga_putchar(*str);
        str++;
    }
}

/*
 * 32-bit tam sayıyı hexadecimal (16'lık) formatta yazar.
 * Örnek: 0x001FADBC
 * Debug ve bellek adresi gösterimi için kullanılır.
 */
void vga_put_hex(uint32_t value)
{
    static const char hex_digits[] = "0123456789ABCDEF";

    vga_puts("0x");

    /* En yüksek nibble'dan başla (28. bit) */
    for (int i = 28; i >= 0; i -= 4) {
        vga_putchar(hex_digits[(value >> i) & 0x0F]);
    }
}

/*
 * 32-bit tam sayıyı ondalık (decimal) formatta yazar.
 * Örnek: 12345
 * Basit bir recursive-free algoritma kullanır.
 */
void vga_put_dec(uint32_t value)
{
    if (value == 0) {
        vga_putchar('0');
        return;
    }

    /* Rakamları ters sırada bir buffer'a topla */
    char buf[12];   /* 32-bit max: 4294967295 = 10 rakam + null */
    int  pos = 0;

    while (value > 0) {
        buf[pos++] = '0' + (value % 10);
        value /= 10;
    }

    /* Ters sırada yazdır (en yüksek basamaktan başla) */
    for (int i = pos - 1; i >= 0; i--) {
        vga_putchar(buf[i]);
    }
}
