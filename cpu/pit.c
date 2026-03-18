#include "pit.h"
#include "ports.h"
#include "isr.h"
#include "pic.h"

#define PIT_CHANNEL0    0x40
#define PIT_CMD         0x43
#define PIT_BASE_FREQ   1193182

static volatile uint32_t pit_ticks = 0;

static void pit_handler(registers_t* regs)
{
    (void)regs;
    pit_ticks++;
}

uint32_t pit_get_ticks(void)
{
    return pit_ticks;
}

void pit_init(uint32_t frequency)
{
    uint32_t divisor = PIT_BASE_FREQ / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1)     divisor = 1;

    outb(PIT_CMD, 0x36);
    outb(PIT_CHANNEL0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_register_handler(IRQ_TIMER, pit_handler);
    pic_clear_mask(IRQ_TIMER);
}
