/*============================================================================
 * WintakOS - kernel.c
 * Kernel Ana Giris Noktasi — Phase 1
 *==========================================================================*/

#ifndef WINTAKOS_TYPES_H
#include "../include/types.h"
#endif

#ifndef WINTAKOS_VGA_H
#include "../kernel/vga.h"
#endif

#ifndef WINTAKOS_GDT_H
#include "../cpu/gdt.h"
#endif

#ifndef WINTAKOS_IDT_H
#include "../cpu/idt.h"
#endif

#ifndef WINTAKOS_ISR_H
#include "../cpu/isr.h"
#endif

#ifndef WINTAKOS_PIC_H
#include "../cpu/pic.h"
#endif

#ifndef WINTAKOS_PIT_H
#include "../cpu/pit.h"
#endif

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289

/*==========================================================================
 * Yardimci Fonksiyonlar
 *========================================================================*/

static void print_banner(void)
{
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  __        ___       _        _     ___  ____  \n");
    vga_puts("  \\ \\      / (_)_ __ | |_ __ _| | __/ _ \\/ ___| \n");
    vga_puts("   \\ \\ /\\ / /| | '_ \\| __/ _` | |/ / | | \\___ \\ \n");
    vga_puts("    \\ V  V / | | | | | || (_| |   <| |_| |___) |\n");
    vga_puts("     \\_/\\_/  |_|_| |_|\\__\\__,_|_|\\_\\\\___/|____/ \n\n");

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Surum: ");
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts("0.2.0");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  (Phase 1 - Interrupt Altyapisi)\n");
    vga_puts("  =========================================================\n\n");
}

static void print_ok(const char* msg)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts(" OK ");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("] ");
    vga_puts(msg);
    vga_putchar('\n');
}

static void print_fail(const char* msg)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [");
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_puts("FAIL");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("] ");
    vga_puts(msg);
    vga_putchar('\n');
}

/*==========================================================================
 * Kernel Ana Fonksiyonu
 *========================================================================*/

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    (void)mbi_ptr;

    /* --- VGA --- */
    vga_init();
    print_banner();

    /* --- Multiboot2 Dogrulama --- */
    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        print_fail("Multiboot2 dogrulama BASARISIZ!");
        vga_puts("    Beklenen: "); vga_put_hex(MULTIBOOT2_BOOTLOADER_MAGIC);
        vga_puts("\n    Alinan:   "); vga_put_hex(magic);
        vga_puts("\n\n  Sistem durduruluyor.\n");
        return;
    }
    print_ok("Multiboot2 dogrulama basarili");

    /* --- Phase 1: Interrupt Altyapisi --- */

    gdt_init();
    print_ok("GDT kuruldu (Null + KernelCode + KernelData)");

    idt_init();
    print_ok("IDT kuruldu (256 giris)");

    pic_init();
    print_ok("PIC yapilandirildi (IRQ 0-15 -> INT 32-47)");

    isr_init();
    print_ok("ISR/IRQ handler'lari yuklendi (48 handler)");

    pit_init(PIT_FREQUENCY);
    print_ok("PIT zamanlayici aktif (100 Hz)");

    /* Interruptlari etkinlestir */
    __asm__ volatile("sti");
    print_ok("Kesme sistemi aktif (STI)");

    /* --- Durum Ozeti --- */
    vga_puts("\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  Phase 1 tamamlandi. Sistem calisyor.\n\n");

    /* Canli sure gostergesi */
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Calisma suresi: ");
    uint8_t timer_row = vga_get_row();
    uint8_t timer_col = vga_get_col();

    vga_puts("\n\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  Sonraki: Phase 2 - PS/2 Klavye + Turkce Q Klavye Duzeni\n");
    vga_puts("  ---------------------------------------------------------\n");

    /* Ana dongu: her saniyede sureyi guncelle */
    uint32_t last_second = 0;

    while (1) {
        uint32_t ticks   = pit_get_ticks();
        uint32_t seconds = ticks / PIT_FREQUENCY;

        if (seconds != last_second) {
            last_second = seconds;

            vga_set_cursor(timer_row, timer_col);
            vga_set_color(VGA_YELLOW, VGA_BLACK);
            vga_put_dec(seconds);
            vga_puts(" saniye    ");
        }

        __asm__ volatile("hlt");
    }
}
