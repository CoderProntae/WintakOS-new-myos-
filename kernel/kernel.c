#include "../include/types.h"
#include "../kernel/vga.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../cpu/pit.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga_font.h"
#include "../drivers/framebuffer.h"
#include "../drivers/fbconsole.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289
#define WINTAKOS_VERSION "0.5.0"

extern uint32_t kernel_end;
#define DEFAULT_MEMORY_KB  (128 * 1024)

static bool graphics_mode = false;

static void kputs(const char* s)
{
    if (graphics_mode) fbcon_puts(s);
    else vga_puts(s);
}

static void kputchar(uint8_t c)
{
    if (graphics_mode) fbcon_putchar(c);
    else vga_putchar(c);
}

static void kput_dec(uint32_t v)
{
    if (graphics_mode) fbcon_put_dec(v);
    else vga_put_dec(v);
}

static void kput_hex(uint32_t v)
{
    if (graphics_mode) fbcon_put_hex(v);
    else vga_put_hex(v);
}

static void kset_color_fb(uint32_t fg, uint32_t bg)
{
    if (graphics_mode) fbcon_set_color(fg, bg);
}

static void print_ok(const char* msg)
{
    if (graphics_mode) {
        fbcon_set_color(COLOR_WHITE, COLOR_BG_DEFAULT);
        fbcon_puts("  [");
        fbcon_set_color(COLOR_GREEN, COLOR_BG_DEFAULT);
        fbcon_puts(" OK ");
        fbcon_set_color(COLOR_WHITE, COLOR_BG_DEFAULT);
        fbcon_puts("] ");
        fbcon_puts(msg);
        fbcon_putchar('\n');
    } else {
        vga_set_color(VGA_WHITE, VGA_BLACK);
        vga_puts("  [");
        vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
        vga_puts(" OK ");
        vga_set_color(VGA_WHITE, VGA_BLACK);
        vga_puts("] ");
        vga_puts(msg);
        vga_putchar('\n');
    }
}

static void print_fail(const char* msg)
{
    if (graphics_mode) {
        fbcon_set_color(COLOR_WHITE, COLOR_BG_DEFAULT);
        fbcon_puts("  [");
        fbcon_set_color(COLOR_RED, COLOR_BG_DEFAULT);
        fbcon_puts("FAIL");
        fbcon_set_color(COLOR_WHITE, COLOR_BG_DEFAULT);
        fbcon_puts("] ");
        fbcon_puts(msg);
        fbcon_putchar('\n');
    } else {
        vga_set_color(VGA_WHITE, VGA_BLACK);
        vga_puts("  [");
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("FAIL");
        vga_set_color(VGA_WHITE, VGA_BLACK);
        vga_puts("] ");
        vga_puts(msg);
        vga_putchar('\n');
    }
}

static void print_info_dec(const char* label, uint32_t value, const char* unit)
{
    kset_color_fb(COLOR_WHITE, COLOR_BG_DEFAULT);
    kputs("         ");
    kputs(label);
    kset_color_fb(COLOR_YELLOW, COLOR_BG_DEFAULT);
    kput_dec(value);
    kset_color_fb(COLOR_LIGHT_GREY, COLOR_BG_DEFAULT);
    kputs(unit);
    kputchar('\n');
}

static void print_info_hex(const char* label, uint32_t value)
{
    kset_color_fb(COLOR_WHITE, COLOR_BG_DEFAULT);
    kputs("         ");
    kputs(label);
    kset_color_fb(COLOR_YELLOW, COLOR_BG_DEFAULT);
    kput_hex(value);
    kputchar('\n');
}

static void print_banner(void)
{
    kset_color_fb(COLOR_CYAN, COLOR_BG_DEFAULT);
    kputs("\n");
    kputs("  __        ___       _        _     ___  ____  \n");
    kputs("  \\ \\      / (_)_ __ | |_ __ _| | __/ _ \\/ ___| \n");
    kputs("   \\ \\ /\\ / /| | '_ \\| __/ _` | |/ / | | \\___ \\ \n");
    kputs("    \\ V  V / | | | | | || (_| |   <| |_| |___) |\n");
    kputs("     \\_/\\_/  |_|_| |_|\\__\\__,_|_|\\_\\\\___/|____/ \n\n");

    kset_color_fb(COLOR_WHITE, COLOR_BG_DEFAULT);
    kputs("  Surum: ");
    kset_color_fb(COLOR_YELLOW, COLOR_BG_DEFAULT);
    kputs(WINTAKOS_VERSION);
    kset_color_fb(COLOR_DARK_GREY, COLOR_BG_DEFAULT);
    kputs("  (Milestone 4 - VESA Grafik Modu)\n");
    kputs("  =========================================================\n\n");
}

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    gdt_init();
    idt_init();
    pic_init();
    isr_init();
    pit_init(PIT_FREQUENCY);

    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC && fb_init(mbi_ptr)) {
        graphics_mode = true;
        fbcon_init();
    } else {
        graphics_mode = false;
        vga_init();
        vga_font_install_turkish();
    }

    print_banner();

    if (magic != MULTIBOOT2_BOOTLOADER_MAGIC) {
        print_fail("Multiboot2 dogrulama BASARISIZ!");
        return;
    }
    print_ok("Multiboot2 dogrulama basarili");
    print_ok("GDT kuruldu");
    print_ok("IDT kuruldu (256 giris)");
    print_ok("PIC yapilandirildi");
    print_ok("ISR/IRQ handler'lari yuklendi");
    print_ok("PIT zamanlayici aktif (100 Hz)");

    keyboard_init();
    print_ok("PS/2 klavye aktif (Turkce Q)");

    if (graphics_mode) {
        framebuffer_t* info = fb_get_info();
        print_ok("VESA framebuffer aktif");
        print_info_dec("Cozunurluk:  ", info->width, "");
        kputs("         x");
        kput_dec(info->height);
        kputs("x");
        kput_dec((uint32_t)info->bpp);
        kputchar('\n');
        print_info_dec("Pitch:       ", info->pitch, " byte/satir");
        print_info_hex("FB Adres:    ", (uint32_t)(uintptr_t)info->address);
    } else {
        print_ok("VGA Text Mode (grafik modu bulunamadi)");
    }

    pmm_init(DEFAULT_MEMORY_KB, (uint32_t)&kernel_end);
    print_ok("PMM baslatildi");
    print_info_dec("Toplam RAM:  ", DEFAULT_MEMORY_KB / 1024, " MB");
    print_info_dec("Bos sayfa:   ", pmm_get_free_pages(), "");

    heap_init();
    print_ok("Kernel heap baslatildi (64 KB)");

    __asm__ volatile("sti");
    print_ok("Kesme sistemi aktif (STI)");

    kputs("\n");
    kset_color_fb(COLOR_DARK_GREY, COLOR_BG_DEFAULT);
    kputs("  ---------------------------------------------------------\n");
    kset_color_fb(COLOR_GREEN, COLOR_BG_DEFAULT);
    kputs("  Milestone 4 tamamlandi.\n\n");

    kset_color_fb(COLOR_WHITE, COLOR_BG_DEFAULT);
    kputs("  Sure: ");

    uint32_t timer_row, timer_col;
    if (graphics_mode) {
        timer_row = fbcon_get_row();
        timer_col = fbcon_get_col();
    } else {
        timer_row = vga_get_row();
        timer_col = vga_get_col();
    }

    kputs("\n\n");
    kset_color_fb(COLOR_DARK_GREY, COLOR_BG_DEFAULT);
    kputs("  Sonraki: Milestone 5 - GUI Framework\n");
    kputs("  ---------------------------------------------------------\n");
    kset_color_fb(COLOR_WHITE, COLOR_BG_DEFAULT);
    kputs("  Klavye testi:\n\n");
    kset_color_fb(COLOR_GREEN, COLOR_BG_DEFAULT);
    kputs("  WintakOS> ");
    kset_color_fb(COLOR_WHITE, COLOR_BG_DEFAULT);

    uint32_t last_second = 0;

    while (1) {
        uint32_t ticks   = pit_get_ticks();
        uint32_t seconds = ticks / PIT_FREQUENCY;

        if (seconds != last_second) {
            last_second = seconds;

            uint32_t cur_row, cur_col;
            if (graphics_mode) {
                cur_row = fbcon_get_row();
                cur_col = fbcon_get_col();
                fbcon_set_cursor(timer_row, timer_col);
                fbcon_set_color(COLOR_YELLOW, COLOR_BG_DEFAULT);
                fbcon_put_dec(seconds);
                fbcon_puts("s    ");
                fbcon_set_cursor(cur_row, cur_col);
                fbcon_set_color(COLOR_WHITE, COLOR_BG_DEFAULT);
            } else {
                cur_row = vga_get_row();
                cur_col = vga_get_col();
                vga_set_cursor((uint8_t)timer_row, (uint8_t)timer_col);
                vga_set_color(VGA_YELLOW, VGA_BLACK);
                vga_put_dec(seconds);
                vga_puts("s    ");
                vga_set_cursor((uint8_t)cur_row, (uint8_t)cur_col);
                vga_set_color(VGA_WHITE, VGA_BLACK);
            }
        }

        uint8_t c = keyboard_getchar();
        if (c) {
            if (c == '\n') {
                kputchar('\n');
                kset_color_fb(COLOR_GREEN, COLOR_BG_DEFAULT);
                kputs("  WintakOS> ");
                kset_color_fb(COLOR_WHITE, COLOR_BG_DEFAULT);
            } else if (c == '\b') {
                kputchar('\b');
            } else {
                kputchar(c);
            }
        }

        __asm__ volatile("hlt");
    }
}
