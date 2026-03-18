/*============================================================================
 * WintakOS - kernel.c
 * Kernel Ana Giriş Noktası (Phase 1 Güncellemesi)
 *
 * Boot sırası:
 *   1. VGA başlat
 *   2. GDT yükle
 *   3. IDT yükle
 *   4. PIC başlat (IRQ remap)
 *   5. PIT başlat (100Hz timer)
 *   6. Interrupt'ları aç (STI)
 *   7. Timer doğrulaması
 *==========================================================================*/

#include "types.h"
#include "vga.h"
#include "gdt.h"
#include "idt.h"
#include "isr.h"
#include "pic.h"
#include "pit.h"

/*--- Multiboot2 Sabitleri ---*/
#define MULTIBOOT2_BOOTLOADER_MAGIC     0x36D76289

/*--- Kernel Sürüm Bilgileri ---*/
#define WINTAKOS_VERSION_MAJOR  0
#define WINTAKOS_VERSION_MINOR  1
#define WINTAKOS_VERSION_PATCH  0

/*==========================================================================
 * Yardımcı Fonksiyonlar
 *========================================================================*/

static void print_banner(void)
{
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  __        ___       _        _     ___  ____  \n");
    vga_puts("  \\ \\      / (_)_ __ | |_ __ _| | __/ _ \\/ ___| \n");
    vga_puts("   \\ \\ /\\ / /| | '_ \\| __/ _` | |/ / | | \\___ \\ \n");
    vga_puts("    \\ V  V / | | | | | || (_| |   <| |_| |___) |\n");
    vga_puts("     \\_/\\_/  |_|_| |_|\\__\\__,_|_|\\_\\\\___/|____/ \n");
    vga_puts("\n");

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Surum: ");
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_put_dec(WINTAKOS_VERSION_MAJOR);
    vga_putchar('.');
    vga_put_dec(WINTAKOS_VERSION_MINOR);
    vga_putchar('.');
    vga_put_dec(WINTAKOS_VERSION_PATCH);
    vga_puts("  (Phase 1 - Interrupt Altyapisi)\n");

    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  =========================================================\n\n");
}

static void print_ok(const char* msg)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [ ");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts(" OK ");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts(" ] ");
    vga_puts(msg);
    vga_putchar('\n');
}

static void print_fail(const char* msg)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [ ");
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_puts("FAIL");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts(" ] ");
    vga_puts(msg);
    vga_putchar('\n');
}

static void print_info(const char* label, uint32_t value)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [ ");
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("INFO");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts(" ] ");
    vga_puts(label);
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_put_dec(value);
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_putchar('\n');
}

/*==========================================================================
 * Kernel Ana Fonksiyonu
 *========================================================================*/
void kernel_main(uint32_t magic, void* mbi_ptr)
{
    (void)mbi_ptr;

    /*=== 1. VGA Başlat ===*/
    vga_init();
    print_banner();

    /*=== 2. Multiboot2 Doğrulama ===*/
    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        print_ok("Multiboot2 dogrulama basarili");
    } else {
        print_fail("Multiboot2 dogrulama BASARISIZ!");
        vga_puts("    Beklenen: 0x36D76289, Alinan: ");
        vga_put_hex(magic);
        vga_puts("\n    Sistem durduruluyor.\n");
        while (1) { __asm__ volatile ("cli; hlt"); }
    }

    /*=== 3. GDT Başlat ===*/
    gdt_init();
    print_ok("GDT yuklendi (5 segment: null, kcode, kdata, ucode, udata)");

    /*=== 4. IDT Başlat ===*/
    idt_init();
    print_ok("IDT yuklendi (48 gate: 32 exception + 16 IRQ)");

    /*=== 5. PIC Başlat ===*/
    pic_init();
    print_ok("PIC baslatildi (IRQ 0-15 → INT 32-47 remap)");

    /*=== 6. PIT Başlat (100 Hz) ===*/
    pit_init(100);
    print_ok("PIT baslatildi (100 Hz, 10ms aralik)");

    /*=== 7. Interrupt'ları Aç ===*/
    __asm__ volatile ("sti");
    print_ok("Interrupt'lar aktif (STI)");

    /*=== 8. PIT Doğrulama ===*/
    /* 20 tick bekle (200ms) ve sayacı kontrol et */
    pit_sleep(20);

    uint32_t ticks = pit_get_ticks();
    if (ticks > 0) {
        print_ok("PIT zamanlayici CALISIYOR");
        print_info("  Tick sayisi: ", ticks);
    } else {
        print_fail("PIT zamanlayici calismadi!");
    }

    /*=== 9. Phase 1 Tamamlandı ===*/
    vga_puts("\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Sonraki adimlar:\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("    > PS/2 klavye surucusu + Turkce Q klavye\n");
    vga_puts("    > Bellek yonetimi (PMM + Heap)\n");
    vga_puts("    > VESA grafik modu (800x600x32)\n");
    vga_puts("\n");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  WintakOS Phase 1 tamamlandi. Interrupt altyapisi hazir.\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");

    /*=== 10. Ana Döngü ===*/
    while (1) {
        __asm__ volatile ("hlt");
    }
}
