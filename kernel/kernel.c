#include "../include/types.h"
#include "../kernel/vga.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../cpu/pit.h"
#include "../cpu/rtc.h"
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
#include "../apps/terminal.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289

extern uint32_t kernel_end;
#define DEFAULT_MEMORY_KB  (128 * 1024)

static window_t* win_welcome = NULL;
static terminal_t* main_terminal = NULL;

static void welcome_draw(window_t* win)
{
    widget_draw_label(win, 16, 16, "WintakOS v0.7.0", RGB(30, 30, 120));
    widget_draw_label(win, 16, 40, "Milestone 6: Terminal/Shell", RGB(80, 80, 80));
    widget_draw_label(win, 16, 72, "Mouse ile pencereleri", RGB(30, 30, 30));
    widget_draw_label(win, 16, 92, "surukleyebilirsiniz.", RGB(30, 30, 30));
    widget_draw_label(win, 16, 120, "Terminal'e komut yazin.", RGB(30, 30, 30));
    widget_draw_button(win, 16, 158, 120, 28, "Tamam", RGB(50, 100, 180), COLOR_WHITE);
}

static void welcome_click(window_t* win, int32_t rx, int32_t ry)
{
    if (widget_button_hit(win, 16, 158, 120, 28, rx, ry)) {
        wm_close_window(win);
    }
}

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    gdt_init();
    idt_init();
    pic_init();
    isr_init();
    pit_init(PIT_FREQUENCY);

    bool graphics_mode = false;
    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC && fb_init(mbi_ptr))
        graphics_mode = true;

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

    pmm_init(DEFAULT_MEMORY_KB, (uint32_t)&kernel_end);
    heap_init();

    framebuffer_t* fb_info = fb_get_info();
    mouse_init(fb_info->width, fb_info->height);
    keyboard_init();

    desktop_init();

    /* Hosgeldiniz penceresi */
    win_welcome = wm_create_window(60, 40, 340, 200, "Hosgeldiniz", RGB(240, 240, 250));
    if (win_welcome) {
        win_welcome->on_draw = welcome_draw;
        win_welcome->on_click = welcome_click;
    }

    /* Terminal penceresi */
    main_terminal = terminal_create(320, 120);

    wm_set_dirty();
    __asm__ volatile("sti");

    while (1) {
        /* Klavye girisi → aktif terminale yonlendir */
        uint8_t c = keyboard_getchar();
        if (c && main_terminal) {
            terminal_input_char(main_terminal, c);
        }

        desktop_update();
        __asm__ volatile("hlt");
    }
}
