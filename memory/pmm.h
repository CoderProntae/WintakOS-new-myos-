#ifndef WINTAKOS_PMM_H
#define WINTAKOS_PMM_H

#include "../include/types.h"

#define PAGE_SIZE       4096
#define MAX_PAGES       1048576     /* 4GB / 4KB = 1M sayfa */
#define BITMAP_SIZE     (MAX_PAGES / 8)  /* 128 KB */

void     pmm_init(uint32_t total_memory_kb, uint32_t kernel_end_addr);
void*    pmm_alloc_page(void);
void     pmm_free_page(void* addr);
uint32_t pmm_get_free_pages(void);
uint32_t pmm_get_total_pages(void);
uint32_t pmm_get_used_pages(void);
uint32_t pmm_get_total_mb(void);

#endif
