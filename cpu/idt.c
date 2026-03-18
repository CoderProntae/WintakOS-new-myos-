#include "idt.h"

typedef struct {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  type_attr;
    uint16_t offset_high;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

static idt_entry_t idt_entries[IDT_ENTRIES];
static idt_ptr_t   idt_ptr;

extern void idt_flush(uint32_t idt_ptr_addr);

void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags)
{
    idt_entries[num].offset_low  = handler & 0xFFFF;
    idt_entries[num].offset_high = (handler >> 16) & 0xFFFF;
    idt_entries[num].selector    = selector;
    idt_entries[num].zero        = 0;
    idt_entries[num].type_attr   = flags;
}

void idt_init(void)
{
    idt_ptr.limit = sizeof(idt_entries) - 1;
    idt_ptr.base  = (uint32_t)&idt_entries;

    for (int i = 0; i < IDT_ENTRIES; i++) {
        idt_set_gate((uint8_t)i, 0, 0, 0);
    }

    idt_flush((uint32_t)&idt_ptr);
}
