/*============================================================================
 * WintakOS - kernel.c
 * Kernel Ana Giriş Noktası
 *
 * boot.asm tarafından çağrılır. GRUB'dan gelen Multiboot2 bilgilerini
 * doğrular ve sistem başlatma sürecini yönetir.
 *
 * İleride bu dosya, tüm alt sistemlerin (GDT, IDT, bellek yönetimi,
 * sürücüler, GUI) başlatılma sırasını koordine edecek.
 *==========================================================================*/

#include "types.h"
#include "vga.h"

/*--- Multiboot2 Sabitleri ---*/
/* GRUB, kernel'i başarıyla yüklediğinde EAX'e bu değeri koyar */
#define MULTIBOOT2_BOOTLOADER_MAGIC     0x36D76289

/*--- Kernel Sürüm Bilgileri ---*/
#define WINTAKOS_VERSION_MAJOR  0
#define WINTAKOS_VERSION_MINOR  1
#define WINTAKOS_VERSION_PATCH  0

/*==========================================================================
 * Yardımcı Fonksiyonlar
 *========================================================================*/

/*
 * Başlık banner'ını yazdırır.
 * Renkli ve düzenli bir hoş geldin mesajı gösterir.
 */
static void print_banner(void)
{
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("  __        ___       _        _     ___  ____  \n");
    vga_puts("  \\ \\      / (_)_ __ | |_ __ _| | __/ _ \\/ ___| \n");
    vga_puts("   \\ \\ /\\ / /| | '_ \\| __/ _` | |/ / | | \\___ \\ \n");
    vga_puts("    \\ V  V / | | | | | || (_| |   <| |_| |___) |\n");
    vga_puts("     \\_/\\_/  |_|_| |_|\\__\\__,_|_|\\_\\\\___/|____/ \n");
    vga_puts("\n");

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Surum: ");
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_put_dec(WINTAKOS_VERSION_MAJOR);
    vga_putchar('.');
    vga_put_dec(WINTAKOS_VERSION_MINOR);
    vga_putchar('.');
    vga_put_dec(WINTAKOS_VERSION_PATCH);
    vga_puts("\n");

    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  =========================================================\n\n");
}

/*
 * Sistem durumu satırını yazdırır.
 * [  OK  ] veya [ FAIL ] prefixli renkli durum mesajları.
 */
static void print_status_ok(const char* message)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [ ");
    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts(" OK ");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts(" ] ");
    vga_puts(message);
    vga_putchar('\n');
}

static void print_status_fail(const char* message)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [ ");
    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_puts("FAIL");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts(" ] ");
    vga_puts(message);
    vga_putchar('\n');
}

static void print_status_info(const char* label, const char* value)
{
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  [ ");
    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_puts("INFO");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts(" ] ");
    vga_puts(label);
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_puts(value);
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_putchar('\n');
}

/*==========================================================================
 * Kernel Ana Fonksiyonu
 *
 * Parametreler (boot.asm tarafından stack üzerinden aktarılır):
 *   magic   - GRUB Multiboot2 doğrulama değeri (0x36D76289 olmalı)
 *   mbi_ptr - Multiboot2 Information Structure'ın bellek adresi
 *             (şu an kullanılmıyor, Phase 1'de parse edilecek)
 *========================================================================*/
void kernel_main(uint32_t magic, void* mbi_ptr)
{
    /* Kullanılmayan parametreyi işaretle (derleyici uyarısını önle) */
    (void)mbi_ptr;

    /*--- 1. VGA Sürücüsünü Başlat ---*/
    vga_init();

    /*--- 2. Banner Yazdır ---*/
    print_banner();

    /*--- 3. Multiboot2 Doğrulama ---*/
    if (magic == MULTIBOOT2_BOOTLOADER_MAGIC) {
        print_status_ok("Multiboot2 dogrulama basarili");
    } else {
        print_status_fail("Multiboot2 dogrulama BASARISIZ!");
        vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
        vga_puts("\n  HATA: Gecersiz magic deger: ");
        vga_put_hex(magic);
        vga_puts("\n  Beklenen: ");
        vga_put_hex(MULTIBOOT2_BOOTLOADER_MAGIC);
        vga_puts("\n\n  Sistem durduruluyor.\n");
        return;
    }

    /*--- 4. Temel Sistem Bilgileri ---*/
    print_status_ok("VGA Text Mode surucusu aktif (80x25)");
    print_status_ok("Kernel stack kuruldu (32 KB)");

    print_status_info("Multiboot2 Info Adresi: ", "");
    vga_put_hex((uint32_t)(uintptr_t)mbi_ptr);
    vga_putchar('\n');

    /*--- 5. Gelecek Fazlar İçin Yer Tutucu ---*/
    vga_puts("\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");
    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_puts("  Sonraki adimlar:\n");
    vga_set_color(VGA_LIGHT_GREY, VGA_BLACK);
    vga_puts("    > GDT (Global Descriptor Table) kurulumu\n");
    vga_puts("    > IDT (Interrupt Descriptor Table) kurulumu\n");
    vga_puts("    > PIC ve PIT yapilandirmasi\n");
    vga_puts("    > PS/2 klavye surucusu\n");
    vga_puts("    > Bellek yonetimi (PMM + Heap)\n");
    vga_puts("\n");

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_puts("  WintakOS Phase 0 tamamlandi. Sistem hazir.\n");
    vga_set_color(VGA_DARK_GREY, VGA_BLACK);
    vga_puts("  ---------------------------------------------------------\n");

    /*--- 6. Sonsuz Döngü ---*/
    /* kernel_main() dönerse boot.asm'deki halt döngüsüne düşer.
     * Ama burada da açıkça bekliyoruz. */
    while (1) {
        __asm__ volatile ("hlt");
    }
}
