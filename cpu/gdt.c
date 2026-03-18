/*============================================================================
 * WintakOS - gdt.c
 * Global Descriptor Table (GDT) - Uygulama
 *==========================================================================*/

#include "gdt.h"

/*--- Assembly fonksiyonu: LGDT çalıştırır ve segment register'ları yükler ---*/
extern void gdt_flush(uint32_t gdt_ptr_addr);

/*--- GDT tablosu ve pointer ---*/
static gdt_entry_t gdt_entries[GDT_ENTRIES];
static gdt_ptr_t   gdt_ptr;

/*==========================================================================
 * Bir GDT girişini ayarla.
 *
 * num         : Giriş indeksi (0-4)
 * base        : Segmentin başlangıç adresi
 * limit       : Segmentin boyutu (granularity'ye bağlı)
 * access      : Erişim hakları byte'ı
 * granularity : Üst 4 bit bayraklar (G, D/B, L, AVL)
 *========================================================================*/
static void gdt_set_gate(int num, uint32_t base, uint32_t limit,
                         uint8_t access, uint8_t gran)
{
    /* Base adresini 3 parçaya böl */
    gdt_entries[num].base_low    = (uint16_t)(base & 0xFFFF);
    gdt_entries[num].base_middle = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[num].base_high   = (uint8_t)((base >> 24) & 0xFF);

    /* Limit'i 2 parçaya böl */
    gdt_entries[num].limit_low   = (uint16_t)(limit & 0xFFFF);

    /* Granularity: üst 4 bit bayraklar, alt 4 bit limit'in üst kısmı */
    gdt_entries[num].granularity = (uint8_t)((limit >> 16) & 0x0F);
    gdt_entries[num].granularity |= (gran & 0xF0);

    /* Erişim hakları */
    gdt_entries[num].access = access;
}

/*==========================================================================
 * GDT'yi başlat.
 *
 * 5 segment tanımlıyoruz (flat model):
 *   0: Null descriptor (x86 zorunluluğu — ilk giriş her zaman null)
 *   1: Kernel Code (0x08) — ring 0, çalıştırılabilir, okunabilir
 *   2: Kernel Data (0x10) — ring 0, yazılabilir
 *   3: User Code   (0x18) — ring 3, çalıştırılabilir, okunabilir
 *   4: User Data   (0x20) — ring 3, yazılabilir
 *
 * Access byte açıklaması:
 *   0x9A = 10011010b → Present=1, DPL=0, Type=1, Exec=1, Read=1
 *   0x92 = 10010010b → Present=1, DPL=0, Type=1, Exec=0, Write=1
 *   0xFA = 11111010b → Present=1, DPL=3, Type=1, Exec=1, Read=1
 *   0xF2 = 11110010b → Present=1, DPL=3, Type=1, Exec=0, Write=1
 *
 * Granularity üst 4 bit:
 *   0xCF = 11001111b → G=1(4KB), D=1(32-bit), L=0, limit_high=0xF
 *   Toplam limit: 0xFFFFF × 4KB = 4GB
 *========================================================================*/
void gdt_init(void)
{
    /* GDT pointer'ı hazırla */
    gdt_ptr.limit = (sizeof(gdt_entry_t) * GDT_ENTRIES) - 1;
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    /*                    idx  base        limit       access  gran  */
    gdt_set_gate(0, 0x00000000, 0x00000000, 0x00, 0x00);  /* Null   */
    gdt_set_gate(1, 0x00000000, 0x000FFFFF, 0x9A, 0xCF);  /* KCode  */
    gdt_set_gate(2, 0x00000000, 0x000FFFFF, 0x92, 0xCF);  /* KData  */
    gdt_set_gate(3, 0x00000000, 0x000FFFFF, 0xFA, 0xCF);  /* UCode  */
    gdt_set_gate(4, 0x00000000, 0x000FFFFF, 0xF2, 0xCF);  /* UData  */

    /* LGDT çalıştır ve segment register'ları yeniden yükle */
    gdt_flush((uint32_t)&gdt_ptr);
}
