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
#include "../fs/ramfs.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289

extern uint32_t kernel_end;

/* Multiboot2'den RAM miktarini oku */
static uint32_t detect_memory_kb(void* mbi_ptr)
{
    if (!mbi_ptr) return 128 * 1024;

    uint8_t* ptr = (uint8_t*)mbi_ptr + 8;
    uint32_t max_addr = 0;

    while (1) {
        uint32_t type = *(uint32_t*)ptr;
        uint32_t size = *(uint32_t*)(ptr + 4);
        if (type == 0) break;

        /* Type 6 = memory map */
        if (type == 6) {
            uint32_t entry_size = *(uint32_t*)(ptr + 8);
            uint32_t entry_ver  = *(uint32_t*)(ptr + 12);
            (void)entry_ver;
            uint8_t* entry = ptr + 16;
            uint8_t* end = ptr + size;

            while (entry < end) {
                uint64_t base = *(uint64_t*)entry;
                uint64_t len  = *(uint64_t*)(entry + 8);
                uint32_t etype = *(uint32_t*)(entry + 16);

                if (etype == 1) { /* Available */
                    uint64_t top = base + len;
                    if (top > 0xFFFFFFFF) top = 0xFFFFFFFF;
                    if ((uint32_t)top > max_addr)
                        max_addr = (uint32_t)top;
                }
                entry += entry_size;
            }
        }

        uint32_t tag_size = (size + 7) & ~7;
        ptr += tag_size;
    }

    if (max_addr == 0) return 128 * 1024;
    return max_addr / 1024;
}

static window_t* win_welcome = NULL;
static terminal_t* main_terminal = NULL;

static void welcome_draw(window_t* win)
{
    widget_draw_label(win, 16, 16, "WintakOS v0.7.0", RGB(30, 30, 120));
    widget_draw_label(win, 16, 40, "Milestone 7: RAMFS", RGB(80, 80, 80));
    widget_draw_label(win, 16, 72, "Terminal'de 'ls' ile", RGB(30, 30, 30));
    widget_draw_label(win, 16, 92, "dosyalari gorun.", RGB(30, 30, 30));
    widget_draw_label(win, 16, 120, "'help' tum komutlar.", RGB(30, 30, 30));
    widget_draw_button(win, 16, 158, 120, 28, "Tamam", RGB(50, 100, 180), COLOR_WHITE);
}

static void welcome_click(window_t* win, int32_t rx, int32_t ry)
{
    if (widget_button_hit(win, 16, 158, 120, 28, rx, ry))
        wm_close_window(win);
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
        while (1) { uint8_t c = keyboard_getchar(); if (c) vga_putchar(c); __asm__ volatile("hlt"); }
    }

    /* Bellek — otomatik algilama */
    uint32_t mem_kb = detect_memory_kb(mbi_ptr);
    pmm_init(mem_kb, (uint32_t)&kernel_end);
    heap_init();

    /* RAMFS */
    ramfs_init();

    /* Input */
    framebuffer_t* fb_info = fb_get_info();
    mouse_init(fb_info->width, fb_info->height);
    keyboard_init();

    /* Desktop */
    desktop_init();

    win_welcome = wm_create_window(60, 40, 300, 200, "Hosgeldiniz", RGB(240, 240, 250));
    if (win_welcome) { win_welcome->on_draw = welcome_draw; win_welcome->on_click = welcome_click; }

    main_terminal = terminal_create(280, 100);

    wm_set_dirty();
    __asm__ volatile("sti");

    while (1) {
        uint8_t c = keyboard_getchar();
        if (c && main_terminal) terminal_input_char(main_terminal, c);
        desktop_update();
        __asm__ volatile("hlt");
    }
}
