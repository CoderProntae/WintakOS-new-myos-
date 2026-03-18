#ifndef WINTAKOS_IDT_H
#define WINTAKOS_IDT_H

#include "../include/types.h"

#define IDT_ENTRIES 256

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t handler, uint16_t selector, uint8_t flags);

#endif
