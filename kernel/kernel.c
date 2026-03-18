/*============================================================================
 * WintakOS - kernel.c
 * Milestone 2: PS/2 Klavye + Turkce Q Duzeni
 *==========================================================================*/

#include "../include/types.h"
#include "../kernel/vga.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../cpu/pit.h"
#include "../drivers/keyboard.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289
#define WINTAKOS_VERSION "0.3.0"

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
    vga_puts(WINTAKOS_VERSION);
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  (Milestone 2 - Klavye Destegi)\n");
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

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    (void)mbi_ptr;

    vga_init();
    print_banner();

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        print_fail("Multiboot2 dogrulama BASARISIZ!");
        return;
    }
    print_ok("Multiboot2 dogrulama basarili");

    /* Milestone 0-1 */
    gdt_init();
    print_ok("GDT kuruldu");

    idt_init();
    print_ok("IDT kuruldu (256 giris)");

    pic_init();
    print_ok("PIC yapilandirildi (IRQ -> INT 32-47)");

    isr_init();
    print_ok("ISR/IRQ handler'lari yuklendi");

    pit_init(PIT_FREQUENCY);
    print_ok("PIT zamanlayici aktif (100 Hz)");

    /* Milestone 2 */
    keyboard_init();
    print_ok("PS/2 klavye surucusu aktif (Turkce Q)");

    __asm__ volatile("sti");
    print_ok("Kesme sistemi aktif (STI)");

    /* Durum */
    vga_puts("\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  Milestone 2 tamamlandi.\n\n");

    /* Calisma suresi */
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Sure: ");
    uint8_t timer_row = vga_get_row();
    uint8_t timer_col = vga_get_col();

    /* Terminal satiri */
    vga_puts("\n\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Klavye testi - yazmaya baslayin:\n\n");

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  WintakOS> ");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    uint8_t prompt_row = vga_get_row();
    uint8_t prompt_col = vga_get_col();
    (void)prompt_row;
    (void)prompt_col;

    /* Ana dongu */
    uint32_t last_second = 0;

    while (1) {
        /* Sure guncelle */
        uint32_t ticks   = pit_get_ticks();
        uint32_t seconds = ticks / PIT_FREQUENCY;

        if (seconds != last_second) {
            last_second = seconds;
            uint8_t cur_row = vga_get_row();
            uint8_t cur_col = vga_get_col();

            vga_set_cursor(timer_row, timer_col);
            vga_set_color(VGA_YELLOW, VGA_BLACK);
            vga_put_dec(seconds);
            vga_puts("s    ");

            vga_set_cursor(cur_row, cur_col);
            vga_set_color(VGA_WHITE, VGA_BLACK);
        }

        /* Klavye okuma */
        char c = keyboard_getchar();
        if (c) {
            if (c == '\n') {
                vga_putchar('\n');
                vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
                vga_puts("  WintakOS> ");
                vga_set_color(VGA_WHITE, VGA_BLACK);
            }
            else if (c == '\b') {
                vga_putchar('\b');
            }
            else {
                vga_putchar(c);
            }
        }

        __asm__ volatile("hlt");
    }
}
