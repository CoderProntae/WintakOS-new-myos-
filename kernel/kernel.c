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
#include "../apps/setup.h"
#include "../apps/calculator.h"
#include "../apps/notepad.h"
#include "../apps/sysmonitor.h"
#include "../apps/filemanager.h"
#include "../fs/ramfs.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289

extern uint32_t kernel_end;

static uint32_t detect_memory_kb(void* mbi_ptr)
{
    if (!mbi_ptr) return 128 * 1024;
    uint8_t* ptr = (uint8_t*)mbi_ptr + 8;
    uint32_t max_addr = 0;
    while (1) {
        uint32_t type = *(uint32_t*)ptr;
        uint32_t size = *(uint32_t*)(ptr + 4);
        if (type == 0) break;
        if (type == 6) {
            uint32_t entry_size = *(uint32_t*)(ptr + 8);
            uint8_t* entry = ptr + 16;
            uint8_t* end = ptr + size;
            while (entry < end) {
                uint64_t base = *(uint64_t*)entry;
                uint64_t len  = *(uint64_t*)(entry + 8);
                uint32_t etype = *(uint32_t*)(entry + 16);
                if (etype == 1) {
                    uint64_t top = base + len;
                    if (top > 0xFFFFFFFF) top = 0xFFFFFFFF;
                    if ((uint32_t)top > max_addr) max_addr = (uint32_t)top;
                }
                entry += entry_size;
            }
        }
        ptr += (size + 7) & ~7;
    }
    return max_addr ? max_addr / 1024 : 128 * 1024;
}

static terminal_t* main_terminal = NULL;
static notepad_t* main_notepad = NULL;

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    gdt_init(); idt_init(); pic_init(); isr_init();
    pit_init(PIT_FREQUENCY);

    bool gfx = (magic == MULTIBOOT2_BOOTLOADER_MAGIC && fb_init(mbi_ptr));
    if (!gfx) {
        vga_init(); vga_font_install_turkish();
        vga_puts("WintakOS: Grafik modu bulunamad\x01.\n");
        keyboard_init();
        __asm__ volatile("sti");
        while (1) { uint8_t c = keyboard_getchar(); if (c) vga_putchar(c); __asm__ volatile("hlt"); }
    }

    uint32_t mem_kb = detect_memory_kb(mbi_ptr);
    pmm_init(mem_kb, (uint32_t)&kernel_end);
    heap_init();
    ramfs_init();

    framebuffer_t* fbi = fb_get_info();
    mouse_init(fbi->width, fbi->height);
    keyboard_init();
    __asm__ volatile("sti");

    desktop_init();
    setup_run();
    desktop_apply_config();

    /* Uygulamalar */
    main_terminal = terminal_create(160, 40);
    calculator_create(10, 80);
    main_notepad = notepad_create(500, 30);
    sysmonitor_create(480, 260);
    filemanager_create(10, 310);

    wm_set_dirty();

    while (1) {
        uint8_t c = keyboard_getchar();
        if (c) {
            window_t* aw = NULL;
            for (uint32_t i = 0; i < wm_get_count(); i++) {
                window_t* w = wm_get_window(i);
                if (w && w->active && (w->flags & WIN_FLAG_VISIBLE)) { aw = w; break; }
            }
            if (aw && main_terminal && aw == main_terminal->win)
                terminal_input_char(main_terminal, c);
            else if (aw && main_notepad && aw == main_notepad->win)
                notepad_input_char(main_notepad, c);
        }

        key_event_t ev;
        if (keyboard_poll(&ev) && !ev.released && ev.keycode != KEY_NONE) {
            window_t* aw = NULL;
            for (uint32_t i = 0; i < wm_get_count(); i++) {
                window_t* w = wm_get_window(i);
                if (w && w->active && (w->flags & WIN_FLAG_VISIBLE)) { aw = w; break; }
            }
            if (aw && main_terminal && aw == main_terminal->win)
                terminal_input_key(main_terminal, ev.keycode);
            else if (aw && main_notepad && aw == main_notepad->win)
                notepad_input_key(main_notepad, ev.keycode);
        }

        desktop_update();
        __asm__ volatile("hlt");
    }
}
