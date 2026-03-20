#include "../include/types.h"
#include "../include/version.h"
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
#include "../drivers/speaker.h"
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
#include "../apps/piano.h"
#include "../apps/network.h"
#include "../fs/ramfs.h"
#include "../net/net.h"

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

static terminal_t*    main_terminal = NULL;
static notepad_t*     main_notepad = NULL;
static piano_t*       main_piano = NULL;

/* Uygulama baslat — start menu ve masaustu ikonlari icin */
static void app_launch(int id)
{
    switch (id) {
        case 0: /* Terminal */
            if (!main_terminal ||
                !(main_terminal->win->flags & WIN_FLAG_VISIBLE)) {
                main_terminal = terminal_create(150, 20);
            } else if (main_terminal->win->flags & WIN_FLAG_MINIMIZED) {
                wm_restore_window(main_terminal->win);
            }
            break;
        case 1: /* Sistem Bilgisi */
            sysmonitor_create(490, 250);
            break;
        case 2: /* Ayarlar */
            setup_run();
            desktop_apply_config();
            break;
        case 3: /* Hesap Makinesi */
            calculator_create(10, 60);
            break;
        case 4: /* Dosya Yoneticisi */
            filemanager_create(10, 310);
            break;
        case 5: /* Not Defteri */
            if (!main_notepad ||
                !(main_notepad->win->flags & WIN_FLAG_VISIBLE)) {
                main_notepad = notepad_create(490, 10);
            } else if (main_notepad->win->flags & WIN_FLAG_MINIMIZED) {
                wm_restore_window(main_notepad->win);
            }
            break;
        case 6: /* Piyano */
            if (!main_piano ||
                !(main_piano->win->flags & WIN_FLAG_VISIBLE)) {
                main_piano = piano_create(200, 330);
            } else if (main_piano->win->flags & WIN_FLAG_MINIMIZED) {
                wm_restore_window(main_piano->win);
            }
            break;
        case 7: /* Ag */
            network_create(10, 160);
            break;
        default: break;
    }
    wm_set_dirty();
}

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    gdt_init(); idt_init(); pic_init(); isr_init();
    pit_init(PIT_FREQUENCY);
    speaker_init();

    bool gfx = (magic == MULTIBOOT2_BOOTLOADER_MAGIC && fb_init(mbi_ptr));
    if (!gfx) {
        vga_init(); vga_font_install_turkish();
        vga_puts("WintakOS: Grafik modu bulunamad\x01.\n");
        keyboard_init();
        __asm__ volatile("sti");
        while (1) {
            uint8_t c = keyboard_getchar();
            if (c) vga_putchar(c);
            __asm__ volatile("hlt");
        }
    }

    uint32_t mem_kb = detect_memory_kb(mbi_ptr);
    pmm_init(mem_kb, (uint32_t)&kernel_end);
    heap_init();
    ramfs_init();

    net_init();

    framebuffer_t* fbi = fb_get_info();
    mouse_init(fbi->width, fbi->height);
    keyboard_init();
    __asm__ volatile("sti");

    sound_startup();

    desktop_init();
    setup_run();
    desktop_apply_config();

    /* Baslangic uygulamalari */
    main_terminal = terminal_create(150, 20);
    calculator_create(10, 60);
    main_notepad = notepad_create(490, 10);
    sysmonitor_create(490, 250);
    filemanager_create(10, 310);
    main_piano = piano_create(200, 330);
    network_create(10, 160);

    wm_set_dirty();

    uint32_t piano_release_tick = 0;
    bool piano_key_held = false;

    while (1) {
        /* Start menu tiklama kontrolu */
        if (desktop_start_menu_open()) {
            mouse_state_t ms = mouse_get_state();
            bool left_now = (ms.buttons & MOUSE_BTN_LEFT) != 0;
            static bool sm_prev_left = false;
            if (left_now && !sm_prev_left) {
                int choice = desktop_start_menu_click(ms.x, ms.y);
                if (choice >= 0) app_launch(choice);
            }
            sm_prev_left = left_now;
        }

        /* Masaustu ikon tiklama */
        {
            mouse_state_t ms = mouse_get_state();
            static bool icon_prev_left = false;
            bool left_now = (ms.buttons & MOUSE_BTN_LEFT) != 0;
            if (left_now && !icon_prev_left && !desktop_start_menu_open()) {
                uint32_t ty = fb_get_info()->height - 32;
                if (ms.y < (int32_t)ty) {
                    /* desktop_icon_click yapisi desktop.c icinde */
                    /* Basit kontrol: sol ustteki ikon alani */
                    /* Terminal ikonu: 30, 60 civari */
                    if (ms.x >= 22 && ms.x < 70 && ms.y >= 56 && ms.y < 100)
                        app_launch(0);
                    /* Dosyalar ikonu: 30, 130 */
                    else if (ms.x >= 22 && ms.x < 70 && ms.y >= 126 && ms.y < 170)
                        app_launch(4);
                    /* Ayarlar ikonu: 30, 200 */
                    else if (ms.x >= 22 && ms.x < 70 && ms.y >= 196 && ms.y < 240)
                        app_launch(2);
                }
            }
            icon_prev_left = left_now;
        }

        /* Klavye girisi */
        uint8_t c = keyboard_getchar();
        if (c) {
            window_t* aw = NULL;
            for (uint32_t i = 0; i < wm_get_count(); i++) {
                window_t* w = wm_get_window(i);
                if (w && w->active &&
                    (w->flags & WIN_FLAG_VISIBLE) &&
                    !(w->flags & WIN_FLAG_MINIMIZED)) {
                    aw = w; break;
                }
            }
            if (aw && main_piano && aw == main_piano->win) {
                piano_input_char(main_piano, c);
                piano_key_held = true;
                piano_release_tick = pit_get_ticks() + 15;
            }
            else if (aw && main_terminal && aw == main_terminal->win)
                terminal_input_char(main_terminal, c);
            else if (aw && main_notepad && aw == main_notepad->win)
                notepad_input_char(main_notepad, c);
        }

        if (piano_key_held && pit_get_ticks() >= piano_release_tick) {
            piano_key_release(main_piano);
            piano_key_held = false;
        }

        /* Ozel tus olaylari */
        key_event_t ev;
        if (keyboard_poll(&ev) && !ev.released && ev.keycode != KEY_NONE) {
            window_t* aw = NULL;
            for (uint32_t i = 0; i < wm_get_count(); i++) {
                window_t* w = wm_get_window(i);
                if (w && w->active &&
                    (w->flags & WIN_FLAG_VISIBLE) &&
                    !(w->flags & WIN_FLAG_MINIMIZED)) {
                    aw = w; break;
                }
            }
            if (aw && main_terminal && aw == main_terminal->win)
                terminal_input_key(main_terminal, ev.keycode);
            else if (aw && main_notepad && aw == main_notepad->win)
                notepad_input_key(main_notepad, ev.keycode);
        }

        net_process();
        desktop_update();
        __asm__ volatile("hlt");
    }
}
