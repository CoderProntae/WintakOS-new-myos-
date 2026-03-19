/*============================================================================
 * WintakOS - heap.c
 * Kernel Heap Allocator
 *
 * First-fit algoritma:
 *   malloc: Yeterli boyutta ilk bos bloku bul, gerekirse bol.
 *   free:   Bloku serbest birak, komsu bos bloklarla birlestir.
 *==========================================================================*/

#include "heap.h"
#include "pmm.h"
#include "../lib/string.h"

/* Blok basligi */
typedef struct heap_block {
    uint32_t             size;   /* Kullanilabilir alan (header haric) */
    bool                 free;   /* Blok bos mu? */
    struct heap_block*   next;   /* Sonraki blok */
} __attribute__((packed)) heap_block_t;

#define BLOCK_HEADER_SIZE  sizeof(heap_block_t)
#define MIN_BLOCK_SIZE     16    /* Bolunmus blogun minimum kullanilabilir alani */

static heap_block_t* heap_start = NULL;
static uint32_t      heap_total = 0;
static uint32_t      heap_used  = 0;

/* Heap'e yeni sayfalar ekle */
static bool heap_grow(uint32_t pages)
{
    for (uint32_t i = 0; i < pages; i++) {
        void* page = pmm_alloc_page();
        if (!page) return false;
    }
    return true;
}

void heap_init(void)
{
    /* PMM'den ardisik sayfalar al */
    void* first_page = NULL;

    for (uint32_t i = 0; i < HEAP_INITIAL_PAGES; i++) {
        void* page = pmm_alloc_page();
        if (!page) return;
        if (i == 0) first_page = page;
    }

    if (!first_page) return;

    /* Ilk bloku olustur */
    heap_start = (heap_block_t*)first_page;
    heap_total = HEAP_INITIAL_PAGES * PAGE_SIZE;

    heap_start->size = heap_total - BLOCK_HEADER_SIZE;
    heap_start->free = true;
    heap_start->next = NULL;

    heap_used = 0;
}

void* kmalloc(size_t size)
{
    if (size == 0) return NULL;

    /* 4-byte hizalama */
    size = (size + 3) & ~3;

    heap_block_t* current = heap_start;

    /* First-fit: yeterli boyutta ilk bos bloku bul */
    while (current) {
        if (current->free && current->size >= size) {
            /* Bolunebilir mi? */
            if (current->size >= size + BLOCK_HEADER_SIZE + MIN_BLOCK_SIZE) {
                /* Bloku bol */
                heap_block_t* new_block = (heap_block_t*)(
                    (uint8_t*)current + BLOCK_HEADER_SIZE + size
                );
                new_block->size = current->size - size - BLOCK_HEADER_SIZE;
                new_block->free = true;
                new_block->next = current->next;

                current->size = size;
                current->next = new_block;
            }

            current->free = false;
            heap_used += current->size;

            /* Sifirla ve dondur */
            void* ptr = (void*)((uint8_t*)current + BLOCK_HEADER_SIZE);
            memset(ptr, 0, current->size);
            return ptr;
        }
        current = current->next;
    }

    /* Yer bulunamadi — heap'i buyut ve tekrar dene */
    if (heap_grow(HEAP_GROW_PAGES)) {
        /* Basit yaklasim: son bloga ekleme */
        /* Ileri fazlarda gelistirilebilir */
    }

    return NULL; /* Bellek yetersiz */
}

void kfree(void* ptr)
{
    if (!ptr) return;

    heap_block_t* block = (heap_block_t*)(
        (uint8_t*)ptr - BLOCK_HEADER_SIZE
    );

    if (block->free) return; /* Zaten serbest */

    block->free = true;
    heap_used -= block->size;

    /* Komsu bos bloklari birlestir (coalescing) */
    heap_block_t* current = heap_start;
    while (current) {
        if (current->free && current->next && current->next->free) {
            current->size += BLOCK_HEADER_SIZE + current->next->size;
            current->next = current->next->next;
            continue; /* Zincirde birden fazla birlestirme olabilir */
        }
        current = current->next;
    }
}

uint32_t heap_get_free(void)
{
    return heap_total - heap_used;
}

uint32_t heap_get_used(void)
{
    return heap_used;
}
