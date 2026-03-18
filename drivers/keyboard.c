/*============================================================================
 * WintakOS - keyboard.c
 * PS/2 Klavye Surucusu
 *
 * IRQ1 uzerinden PS/2 klavyeden scancode okur,
 * Turkce Q klavye haritasiyla ASCII'ye cevirir.
 *==========================================================================*/

#include "keyboard.h"
#include "keymap_tr.h"
#include "../cpu/ports.h"
#include "../cpu/isr.h"
#include "../cpu/pic.h"

#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64

/* Dairesel karakter tamponu */
#define KBD_BUFFER_SIZE  256

static char kbd_buffer[KBD_BUFFER_SIZE];
static volatile uint32_t kbd_read_pos  = 0;
static volatile uint32_t kbd_write_pos = 0;

/* Son olay */
static volatile key_event_t last_event;
static volatile bool        event_ready = false;

/* Modifier durumlari */
static key_modifiers_t modifiers = {0, 0, 0, 0, 0};

/* Tampona karakter ekle */
static void kbd_buffer_put(char c)
{
    uint32_t next = (kbd_write_pos + 1) % KBD_BUFFER_SIZE;
    if (next != kbd_read_pos) {
        kbd_buffer[kbd_write_pos] = c;
        kbd_write_pos = next;
    }
}

/* Scancode'u ozel tus koduna cevir */
static uint8_t scancode_to_keycode(uint8_t sc)
{
    switch (sc) {
        case 0x01: return KEY_ESCAPE;
        case 0x0E: return KEY_BACKSPACE;
        case 0x0F: return KEY_TAB;
        case 0x1C: return KEY_ENTER;
        case 0x1D: return KEY_LCTRL;
        case 0x2A: return KEY_LSHIFT;
        case 0x36: return KEY_RSHIFT;
        case 0x38: return KEY_LALT;
        case 0x3A: return KEY_CAPSLOCK;
        case 0x3B: return KEY_F1;
        case 0x3C: return KEY_F2;
        case 0x3D: return KEY_F3;
        case 0x3E: return KEY_F4;
        case 0x3F: return KEY_F5;
        case 0x40: return KEY_F6;
        case 0x41: return KEY_F7;
        case 0x42: return KEY_F8;
        case 0x43: return KEY_F9;
        case 0x44: return KEY_F10;
        case 0x45: return KEY_NUMLOCK;
        case 0x46: return KEY_SCROLLLOCK;
        case 0x57: return KEY_F11;
        case 0x58: return KEY_F12;
        default:   return KEY_NONE;
    }
}

/* IRQ1 handler — her tus basildiginda cagrilir */
static void keyboard_handler(registers_t* regs)
{
    (void)regs;

    uint8_t scancode = inb(KBD_DATA_PORT);
    uint8_t released = scancode & 0x80;
    uint8_t sc       = scancode & 0x7F;

    key_event_t ev;
    ev.ascii    = 0;
    ev.keycode  = KEY_NONE;
    ev.scancode = scancode;
    ev.released = released ? 1 : 0;

    /* Modifier guncelle */
    if (sc == 0x2A || sc == 0x36) {
        modifiers.shift = released ? 0 : 1;
        ev.keycode = (sc == 0x2A) ? KEY_LSHIFT : KEY_RSHIFT;
    }
    else if (sc == 0x1D) {
        modifiers.ctrl = released ? 0 : 1;
        ev.keycode = KEY_LCTRL;
    }
    else if (sc == 0x38) {
        modifiers.alt = released ? 0 : 1;
        ev.keycode = KEY_LALT;
    }
    else if (sc == 0x3A && !released) {
        modifiers.capslock = !modifiers.capslock;
        ev.keycode = KEY_CAPSLOCK;
    }
    else if (sc == 0x45 && !released) {
        modifiers.numlock = !modifiers.numlock;
        ev.keycode = KEY_NUMLOCK;
    }
    else if (!released) {
        /* Normal tus basildi */
        uint8_t kc = scancode_to_keycode(sc);

        if (kc != KEY_NONE) {
            ev.keycode = kc;

            /* Enter, backspace, tab icin ASCII ata */
            if (kc == KEY_ENTER)     ev.ascii = '\n';
            else if (kc == KEY_BACKSPACE) ev.ascii = '\b';
            else if (kc == KEY_TAB)  ev.ascii = '\t';
        }
        else {
            /* Harf/sayi tusu — haritadan bak */
            bool use_shift = modifiers.shift;

            /* CapsLock: sadece harfleri etkiler */
            if (modifiers.capslock && sc < 128) {
                char normal = scancode_to_ascii[sc];
                if (normal >= 'a' && normal <= 'z') {
                    use_shift = !use_shift;
                }
            }

            if (use_shift) {
                ev.ascii = (sc < 128) ? scancode_to_ascii_shift[sc] : 0;
            } else {
                ev.ascii = (sc < 128) ? scancode_to_ascii[sc] : 0;
            }
        }

        /* Tampona ekle */
        if (ev.ascii) {
            kbd_buffer_put(ev.ascii);
        }
    }

    ev.mods = modifiers;
    last_event = ev;
    event_ready = true;
}

void keyboard_init(void)
{
    kbd_read_pos  = 0;
    kbd_write_pos = 0;
    event_ready   = false;

    /* Klavye tamponunu temizle */
    while (inb(KBD_STATUS_PORT) & 0x01) {
        inb(KBD_DATA_PORT);
    }

    irq_register_handler(IRQ_KEYBOARD, keyboard_handler);
    pic_clear_mask(IRQ_KEYBOARD);
}

char keyboard_getchar(void)
{
    if (kbd_read_pos == kbd_write_pos) {
        return 0;
    }
    char c = kbd_buffer[kbd_read_pos];
    kbd_read_pos = (kbd_read_pos + 1) % KBD_BUFFER_SIZE;
    return c;
}

bool keyboard_poll(key_event_t* event)
{
    if (!event_ready) return false;
    event_ready = false;
    *event = last_event;
    return true;
}

key_modifiers_t keyboard_get_modifiers(void)
{
    return modifiers;
}
