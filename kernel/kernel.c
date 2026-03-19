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
#include "../drivers/mouse.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../gui/desktop.h"
#include "../gui/window.h"
#include "../gui/widget.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289
#define WINTAKOS_VERSION "0.6.0"

extern uint32_t kernel_end;
#define DEFAULT_MEMORY_KB  (128 * 1024)

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    /* Temel altyapi */
    gdt_init();
    idt_init();
    pic_init();
    isr_init();
    pit_init(PIT_FREQUENCY);

    /* Framebuffer */
    bool graphics_mode = false;
    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC && fb_init(mbi_ptr)) {
        graphics_mode = true;
    }

    if (!graphics_mode) {
        /* Fallback: VGA text mode */
        vga_init();
        vga_font_install_turkish();
        vga_puts("WintakOS: Grafik modu bulunamadi. VGA text modunda.\n");

        keyboard_init();
        __asm__ volatile("sti");

        while (1) {
            uint8_t c = keyboard_getchar();
            if (c) vga_putchar(c);
            __asm__ volatile("hlt");
        }
    }

    /* Bellek */
    pmm_init(DEFAULT_MEMORY_KB, (uint32_t)&kernel_end);
    heap_init();

    /* Mouse */
    framebuffer_t* fb_info = fb_get_info();
    mouse_init(fb_info->width, fb_info->height);

    /* Klavye */
    keyboard_init();

    /* Masaustu */
    desktop_init();

    /* Demo pencereleri */
    window_t* win1 = wm_create_window(100, 80, 320, 200, "Hosgeldiniz", RGB(240, 240, 245));
    window_t* win2 = wm_create_window(300, 160, 280, 180, "Sistem Bilgisi", RGB(235, 245, 235));

    /* Ilk cizim */
    desktop_draw();

    if (win1) {
        widget_draw_label(win1, 16, 16, "WintakOS v0.6.0", RGB(30, 30, 30));
        widget_draw_label(win1, 16, 40, "Milestone 5: GUI Framework", RGB(80, 80, 80));
        widget_draw_label(win1, 16, 72, "Mouse ile pencereleri", RGB(30, 30, 30));
        widget_draw_label(win1, 16, 92, "surukleyebilirsiniz.", RGB(30, 30, 30));
        widget_draw_button(win1, 16, 150, 120, 30, "Tamam", RGB(50, 100, 180), COLOR_WHITE);
    }

    if (win2) {
        widget_draw_label(win2, 16, 16, "CPU: i386 Protected Mode", RGB(30, 30, 30));
        widget_draw_label(win2, 16, 36, "RAM: 128 MB", RGB(30, 30, 30));
        widget_draw_label(win2, 16, 56, "Video: 800x600x32", RGB(30, 30, 30));
        widget_draw_label(win2, 16, 76, "Klavye: Turkce Q", RGB(30, 30, 30));
        widget_draw_label(win2, 16, 96, "Mouse: PS/2", RGB(30, 30, 30));
    }

    /* Interruptlari ac */
    __asm__ volatile("sti");

    /* Ana dongu */
    uint32_t frame_counter = 0;

    while (1) {
        frame_counter++;

        /* Her 2 tick'te bir guncelle (~50 fps) */
        if (frame_counter % 2 == 0) {
            desktop_update();
        }

        __asm__ volatile("hlt");
    }
}
