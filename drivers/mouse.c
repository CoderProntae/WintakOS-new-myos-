#include "mouse.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"

#define MOUSE_DATA    0x60
#define MOUSE_STATUS  0x64
#define MOUSE_CMD     0x64

static volatile mouse_state_t state;
static uint32_t scr_w = 800;
static uint32_t scr_h = 600;
static volatile uint8_t mouse_cycle = 0;
static volatile int8_t  mouse_bytes[3];

static void mouse_wait_write(void)
{
    int timeout = 100000;
    while (timeout-- > 0) {
        if (!(inb(MOUSE_STATUS) & 0x02)) return;
    }
}

static void mouse_wait_read(void)
{
    int timeout = 100000;
    while (timeout-- > 0) {
        if (inb(MOUSE_STATUS) & 0x01) return;
    }
}

static void mouse_write(uint8_t data)
{
    mouse_wait_write();
    outb(MOUSE_CMD, 0xD4);
    mouse_wait_write();
    outb(MOUSE_DATA, data);
}

static uint8_t mouse_read(void)
{
    mouse_wait_read();
    return inb(MOUSE_DATA);
}

static void mouse_handler(registers_t* regs)
{
    (void)regs;

    uint8_t status = inb(MOUSE_STATUS);
    if (!(status & 0x20)) return;

    int8_t data = (int8_t)inb(MOUSE_DATA);

    switch (mouse_cycle) {
        case 0:
            mouse_bytes[0] = data;
            if (data & 0x08) mouse_cycle = 1;
            break;
        case 1:
            mouse_bytes[1] = data;
            mouse_cycle = 2;
            break;
        case 2:
            mouse_bytes[2] = data;
            mouse_cycle = 0;

            state.buttons = mouse_bytes[0] & 0x07;

            int32_t dx = mouse_bytes[1];
            int32_t dy = -mouse_bytes[2];

            state.x += dx;
            state.y += dy;

            if (state.x < 0) state.x = 0;
            if (state.y < 0) state.y = 0;
            if (state.x >= (int32_t)scr_w) state.x = (int32_t)scr_w - 1;
            if (state.y >= (int32_t)scr_h) state.y = (int32_t)scr_h - 1;

            state.moved = true;
            break;
    }
}

void mouse_init(uint32_t screen_w, uint32_t screen_h)
{
    scr_w = screen_w;
    scr_h = screen_h;
    state.x = (int32_t)(screen_w / 2);
    state.y = (int32_t)(screen_h / 2);
    state.buttons = 0;
    state.moved = false;
    mouse_cycle = 0;

    mouse_wait_write();
    outb(MOUSE_CMD, 0xA8);

    mouse_wait_write();
    outb(MOUSE_CMD, 0x20);
    mouse_wait_read();
    uint8_t status_byte = inb(MOUSE_DATA);
    status_byte |= 0x02;
    status_byte &= ~0x20;
    mouse_wait_write();
    outb(MOUSE_CMD, 0x60);
    mouse_wait_write();
    outb(MOUSE_DATA, status_byte);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();

    irq_register_handler(IRQ_MOUSE, mouse_handler);
    pic_clear_mask(IRQ_MOUSE);
}

mouse_state_t mouse_get_state(void)
{
    return state;
}

void mouse_clear_moved(void)
{
    state.moved = false;
}
