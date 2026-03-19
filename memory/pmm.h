#ifndef WINTAKOS_PMM_H
#define WINTAKOS_PMM_H

#include "../include/types.h"

/*============================================================================
 * Physical Memory Manager
 *
 * Fiziksel RAM'i 4KB (PAGE_SIZE) bloklar halinde yonetir.
 * Bitmap tabanlı: her bit bir sayfayi temsil eder.
 *   0 = sayfa bos (kullanilabilir)
 *   1 = sayfa kullaniliyor veya rezerve
 *
 * Maksimum desteklenen RAM: 128MB (32768 sayfa × 4KB)
 * Bitmap boyutu: 32768 / 8 = 4096 byte = 1 sayfa
 *==========================================================================*/

#define PAGE_SIZE       4096
#define MAX_MEMORY      (128 * 1024 * 1024)   /* 128 MB */
#define MAX_PAGES       (MAX_MEMORY / PAGE_SIZE)
#define BITMAP_SIZE     (MAX_PAGES / 8)

void     pmm_init(uint32_t total_memory_kb, uint32_t kernel_end_addr);
void*    pmm_alloc_page(void);
void     pmm_free_page(void* addr);
uint32_t pmm_get_free_pages(void);
uint32_t pmm_get_total_pages(void);
uint32_t pmm_get_used_pages(void);

#endif
