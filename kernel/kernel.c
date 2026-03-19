#include "../include/types.h"
#include "../kernel/vga.h"
#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"
#include "../cpu/pit.h"
#include "../drivers/keyboard.h"
#include "../drivers/vga_font.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"

#define MULTIBOOT2_BOOTLOADER_MAGIC  0x36D76289
#define WINTAKOS_VERSION "0.4.0"

/* Linker script'ten kernel bitiş adresi */
extern uint32_t kernel_end;

/* Varsayılan bellek: 128MB (Multiboot2 parse ileride eklenecek) */
#define DEFAULT_MEMORY_KB  (128 * 1024)

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
    vga_puts("  (Milestone 3 - Bellek Yonetimi)\n");
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

static void print_info(const char* label, uint32_t value, const char* unit)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("         ");
    vga_puts(label);
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_put_dec(value);
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts(unit);
    vga_putchar('\n');
}

/* Basit bellek testi */
static void memory_test(void)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("\n  --- Bellek Testi ---\n");

    /* Test 1: malloc/free */
    uint32_t* ptr1 = (uint32_t*)kmalloc(64);
    uint32_t* ptr2 = (uint32_t*)kmalloc(128);
    uint32_t* ptr3 = (uint32_t*)kmalloc(256);

    if (ptr1 && ptr2 && ptr3) {
        /* Yazma testi */
        ptr1[0] = 0xDEADBEEF;
        ptr2[0] = 0xCAFEBABE;
        ptr3[0] = 0x12345678;

        /* Okuma dogrulama */
        if (ptr1[0] == 0xDEADBEEF &&
            ptr2[0] == 0xCAFEBABE &&
            ptr3[0] == 0x12345678) {
            print_ok("malloc/free: yazma-okuma basarili");
        } else {
            print_fail("malloc/free: veri bozuldu!");
        }

        print_info("ptr1 adresi: 0x", (uint32_t)(uintptr_t)ptr1, "");
        print_info("ptr2 adresi: 0x", (uint32_t)(uintptr_t)ptr2, "");
        print_info("ptr3 adresi: 0x", (uint32_t)(uintptr_t)ptr3, "");

        /* Heap durumu (free oncesi) */
        print_info("Heap kullanim: ", heap_get_used(), " byte");

        /* Serbest birak */
        kfree(ptr1);
        kfree(ptr2);
        kfree(ptr3);

        /* Heap durumu (free sonrasi) */
        print_info("Free sonrasi:  ", heap_get_used(), " byte");

        if (heap_get_used() == 0) {
            print_ok("malloc/free: tum bellek serbest birakildi");
        }
    } else {
        print_fail("malloc BASARISIZ - bellek ayrilamadi!");
    }

    /* Test 2: PMM sayfa ayirma */
    void* page1 = pmm_alloc_page();
    void* page2 = pmm_alloc_page();

    if (page1 && page2) {
        print_ok("PMM sayfa ayirma basarili");
        print_info("Sayfa 1: 0x", (uint32_t)(uintptr_t)page1, "");
        print_info("Sayfa 2: 0x", (uint32_t)(uintptr_t)page2, "");
        pmm_free_page(page1);
        pmm_free_page(page2);
        print_ok("PMM sayfa serbest birakma basarili");
    } else {
        print_fail("PMM sayfa ayirma BASARISIZ!");
    }
}

void kernel_main(uint32_t magic, void* mbi_ptr)
{
    (void)mbi_ptr;

    vga_init();
    vga_font_install_turkish();
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
    print_ok("PIC yapilandirildi");

    isr_init();
    print_ok("ISR/IRQ handler'lari yuklendi");

    pit_init(PIT_FREQUENCY);
    print_ok("PIT zamanlayici aktif (100 Hz)");

    /* Milestone 2 */
    keyboard_init();
    print_ok("PS/2 klavye aktif (Turkce Q)");
    print_ok("Turkce font glifleri yuklendi");

    /* Milestone 3 */
    pmm_init(DEFAULT_MEMORY_KB, (uint32_t)&kernel_end);
    print_ok("PMM baslatildi (bitmap tabanli)");
    print_info("Toplam RAM:     ", DEFAULT_MEMORY_KB / 1024, " MB");
    print_info("Toplam sayfa:   ", pmm_get_total_pages(), "");
    print_info("Bos sayfa:      ", pmm_get_free_pages(), "");
    print_info("Kullanilan:     ", pmm_get_used_pages(), "");

    heap_init();
    print_ok("Kernel heap baslatildi (64 KB)");
    print_info("Heap toplam:    ", heap_get_free() + heap_get_used(), " byte");
    print_info("Heap bos:       ", heap_get_free(), " byte");

    __asm__ volatile("sti");
    print_ok("Kesme sistemi aktif (STI)");

    /* Bellek testi */
    memory_test();

    /* Durum */
    vga_puts("\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  Milestone 3 tamamlandi.\n\n");

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Sure: ");
    uint8_t timer_row = vga_get_row();
    uint8_t timer_col = vga_get_col();

    vga_puts("\n\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  Sonraki: Milestone 4 - VESA Grafik Modu\n");
    vga_puts("  ---------------------------------------------------------\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Klavye testi:\n\n");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  WintakOS> ");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    uint32_t last_second = 0;

    while (1) {
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

        uint8_t c = keyboard_getchar();
        if (c) {
            if (c == '\n') {
                vga_putchar('\n');
                vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
                vga_puts("  WintakOS> ");
                vga_set_color(VGA_WHITE, VGA_BLACK);
            } else if (c == '\b') {
                vga_putchar('\b');
            } else {
                vga_putchar(c);
            }
        }

        __asm__ volatile("hlt");
    }
}
