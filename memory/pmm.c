/*============================================================================
 * WintakOS - pmm.c
 * Physical Memory Manager - Bitmap Tabanli
 *
 * Bellek haritasi:
 *   0x00000000 - 0x000FFFFF : BIOS/VGA/Legacy (1MB) → REZERVE
 *   0x00100000 - kernel_end : Kernel kodu ve verisi → REZERVE
 *   kernel_end - RAM sonu   : Kullanilabilir alan
 *==========================================================================*/

#include "pmm.h"
#include "../lib/string.h"

static uint8_t  pmm_bitmap[BITMAP_SIZE];
static uint32_t pmm_total_pages  = 0;
static uint32_t pmm_used_pages   = 0;

/* Bitmap yardimcilari */
static inline void bitmap_set(uint32_t page)
{
    pmm_bitmap[page / 8] |= (1 << (page % 8));
}

static inline void bitmap_clear(uint32_t page)
{
    pmm_bitmap[page / 8] &= ~(1 << (page % 8));
}

static inline bool bitmap_test(uint32_t page)
{
    return (pmm_bitmap[page / 8] & (1 << (page % 8))) != 0;
}

/* Ilk bos sayfayi bul */
static int32_t bitmap_find_free(void)
{
    for (uint32_t i = 0; i < pmm_total_pages / 8; i++) {
        if (pmm_bitmap[i] != 0xFF) {
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (!(pmm_bitmap[i] & (1 << bit))) {
                    return (int32_t)(i * 8 + bit);
                }
            }
        }
    }
    return -1;
}

void pmm_init(uint32_t total_memory_kb, uint32_t kernel_end_addr)
{
    uint32_t total_memory = total_memory_kb * 1024;

    /* Maksimum desteklenen bellek siniri */
    if (total_memory > MAX_MEMORY) {
        total_memory = MAX_MEMORY;
    }

    pmm_total_pages = total_memory / PAGE_SIZE;

    /* Baslangicta tum sayfalari kullaniliyor olarak isaretle */
    memset(pmm_bitmap, 0xFF, BITMAP_SIZE);
    pmm_used_pages = pmm_total_pages;

    /* Kernel sonrasindan itibaren sayfalari serbest birak */
    /* kernel_end_addr'i sayfa sinirini yukariya yuvarla */
    uint32_t first_free_page = (kernel_end_addr + PAGE_SIZE - 1) / PAGE_SIZE;

    for (uint32_t i = first_free_page; i < pmm_total_pages; i++) {
        bitmap_clear(i);
        pmm_used_pages--;
    }
}

void* pmm_alloc_page(void)
{
    int32_t page = bitmap_find_free();
    if (page < 0) {
        return NULL; /* Bellek tukendi */
    }

    bitmap_set((uint32_t)page);
    pmm_used_pages++;

    return (void*)((uint32_t)page * PAGE_SIZE);
}

void pmm_free_page(void* addr)
{
    uint32_t page = (uint32_t)addr / PAGE_SIZE;

    if (page >= pmm_total_pages) return;
    if (!bitmap_test(page)) return; /* Zaten serbest */

    bitmap_clear(page);
    pmm_used_pages--;
}

uint32_t pmm_get_free_pages(void)
{
    return pmm_total_pages - pmm_used_pages;
}

uint32_t pmm_get_total_pages(void)
{
    return pmm_total_pages;
}

uint32_t pmm_get_used_pages(void)
{
    return pmm_used_pages;
}
