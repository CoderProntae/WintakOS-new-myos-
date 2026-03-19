#ifndef WINTAKOS_HEAP_H
#define WINTAKOS_HEAP_H

#include "../include/types.h"

/*============================================================================
 * Kernel Heap - malloc/free
 *
 * Linked-list tabanli basit heap allocator.
 * Her blok bir header icrir:
 *   - size: kullanilabilir alan boyutu (byte)
 *   - free: blok bos mu?
 *   - next: sonraki blok pointer'i
 *
 * Heap, PMM'den sayfa sayfa buyur.
 * Serbest birakilan bitisik bloklar birlestirilir (coalescing).
 *==========================================================================*/

#define HEAP_INITIAL_PAGES  16    /* Baslangic: 64 KB */
#define HEAP_GROW_PAGES     4     /* Her genisleme: 16 KB */

void  heap_init(void);
void* kmalloc(size_t size);
void  kfree(void* ptr);

/* Debug: heap durumunu goster */
uint32_t heap_get_free(void);
uint32_t heap_get_used(void);

#endif
