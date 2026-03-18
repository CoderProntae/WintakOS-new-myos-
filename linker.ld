/*============================================================================
 * WintakOS - Linker Script
 * 
 * Kernel'i 1MB adresinden başlatır (Multiboot standardı).
 * Bölümleri (section) doğru sırada ve hizalamada yerleştirir.
 *
 * Neden 1MB? Çünkü 0x00000 - 0xFFFFF arası (ilk 1MB) geleneksel olarak
 * BIOS, VGA belleği ve legacy donanım tarafından kullanılır.
 * Kernel'in güvenli alanı 1MB'dan itibaren başlar.
 *==========================================================================*/

ENTRY(_start)

SECTIONS
{
    /* Kernel başlangıç adresi: 1 MB */
    . = 1M;

    /*----------------------------------------------------------------------
     * Multiboot2 başlığı mutlaka ilk bölüm olmalı.
     * GRUB, ELF dosyasının ilk 32KB'ı içinde bu başlığı arar.
     * 8-byte hizalama Multiboot2 spesifikasyonu gereğidir.
     *--------------------------------------------------------------------*/
    .multiboot2 ALIGN(8) :
    {
        *(.multiboot2)
    }

    /*----------------------------------------------------------------------
     * Çalıştırılabilir kod bölümü
     * 4KB hizalama, sayfa tabloları için uygunluk sağlar.
     *--------------------------------------------------------------------*/
    .text ALIGN(4K) :
    {
        *(.text)
    }

    /*----------------------------------------------------------------------
     * Salt okunur veriler (string sabitleri, const diziler vb.)
     *--------------------------------------------------------------------*/
    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }

    /*----------------------------------------------------------------------
     * Başlatılmış (initialized) global/static değişkenler
     *--------------------------------------------------------------------*/
    .data ALIGN(4K) :
    {
        *(.data)
    }

    /*----------------------------------------------------------------------
     * Başlatılmamış (uninitialized) global/static değişkenler
     * BSS bölümü dosyada yer kaplamaz, bellekte sıfırlanır.
     *--------------------------------------------------------------------*/
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }

    /*----------------------------------------------------------------------
     * Kernel'in bittiği bellek adresi.
     * İleride heap allocator bu adresten itibaren bellek yönetecek.
     *--------------------------------------------------------------------*/
    kernel_end = .;
}
