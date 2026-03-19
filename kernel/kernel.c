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

/* Pencere pointer'lari (icerik cizimi icin) */
static window_t* win_welcome = NULL;
static window_t* win_sysinfo = NULL;

/* Pencere iceriklerini ciz — dirty olunca cagirilir */
static void draw_window_contents(void)
{
    if (win_welcome && (win_welcome->flags & WIN_FLAG_VISIBLE)) {
        widget_draw_label(win_welcome, 16, 16,
            "WintakOS v0.6.0", RGB(30, 30, 120));
        widget_draw_label(win_welcome, 16, 40,
            "Milestone 5: GUI Framework", RGB(80, 80, 80));
        widget_draw_label(win_welcome, 16, 72,
            "Mouse ile pencereleri", RGB(30, 30, 30));
        widget_draw_label(win_welcome, 16, 92,
            "surukleyebilirsiniz.", RGB(30, 30, 30));
        widget_draw_label(win_welcome, 16, 120,
            "X ile pencereyi kapatin.", RGB(30, 30, 30));
        widget_draw_button(win_welcome, 16, 158, 120, 28,
            "Tamam", RGB(50, 100, 180), COLOR_WHITE);
    }

    if (win_sysinfo && (win_sysinfo->flags & WIN_FLAG_VISIBLE)) {
        widget_draw_label(win_sysinfo, 16, 16,
            "CPU: i386 Protected Mode", RGB(30, 80, 30));
        widget_draw_label(win_sysinfo, 16, 40,
            "RAM: 128 MB", RGB(30, 30, 30));
        widget_draw_label(win_sysinfo, 16, 64,
            "Video: 800x600x32 VESA", RGB(30, 30, 30));
        widget_draw_label(win_sysinfo, 16, 88,
            "Klavye: PS/2 Turkce Q", RGB(30, 30, 30));
        widget_draw_label(win_sysinfo, 16, 112,
            "Mouse: PS/2", RGB(30, 30, 30));
        widget_draw_label(win_sysinfo, 16, 144,
            "Heap: 64 KB", RGB(80, 80, 80));
    }
}

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    /* Core systems */
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
        vga_init();
        vga_font_install_turkish();
        vga_puts("WintakOS: Grafik modu bulunamadi.\n");
        keyboard_init();
        __asm__ volatile("sti");
        while (1) {
            uint8_t c = keyboard_getchar();
            if (c) vga_putchar(c);
            __asm__ volatile("hlt");
        }
    }

    /* Memory */
    pmm_init(DEFAULT_MEMORY_KB, (uint32_t)&kernel_end);
    heap_init();

    /* Input */
    framebuffer_t* fb_info = fb_get_info();
    mouse_init(fb_info->width, fb_info->height);
    keyboard_init();

    /* Desktop */
    desktop_init();
    desktop_set_content_drawer(draw_window_contents);

    /* Create demo windows */
    win_welcome = wm_create_window(80, 60, 340, 200,
        "Hosgeldiniz", RGB(240, 240, 250));
    win_sysinfo = wm_create_window(280, 140, 300, 180,
        "Sistem Bilgisi", RGB(235, 250, 235));

    /* Initial draw */
    wm_set_dirty();

    /* Enable interrupts */
    __asm__ volatile("sti");

    /* Main loop */
    while (1) {
        desktop_update();
        __asm__ volatile("hlt");
    }
}
