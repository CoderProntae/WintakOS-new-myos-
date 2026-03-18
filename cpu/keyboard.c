/*============================================================================
 * WintakOS - keyboard.c
 * PS/2 Klavye Sürücüsü
 *==========================================================================*/

#include "keyboard.h"
#include "keymap_tr.h"
#include "ports.h"
#include "isr.h"
#include "pic.h"

#define KBD_DATA_PORT   0x60
#define KBD_STATUS_PORT 0x64

static keyboard_state_t kbd_state = { false, false, false, false };
static keyboard_callback_t kbd_callback = NULL;

keyboard_state_t keyboard_get_state(void)
{
    return kbd_state;
}

void keyboard_set_callback(keyboard_callback_t cb)
{
    kbd_callback = cb;
}

static void keyboard_handler(registers_t* regs)
{
    (void)regs;

    uint8_t scancode = inb(KBD_DATA_PORT);

    /* --- Tuş bırakma (release): bit 7 set --- */
    if (scancode & 0x80) {
        uint8_t released = scancode & 0x7F;
        switch (released) {
            case 0x2A: /* Left Shift release */
            case 0x36: /* Right Shift release */
                kbd_state.shift_pressed = false;
                break;
            case 0x1D: /* Ctrl release */
                kbd_state.ctrl_pressed = false;
                break;
            case 0x38: /* Alt release */
                kbd_state.alt_pressed = false;
                break;
        }
        return;
    }

    /* --- Tuş basma (press) --- */
    switch (scancode) {
        case 0x2A: /* Left Shift */
        case 0x36: /* Right Shift */
            kbd_state.shift_pressed = true;
            return;
        case 0x1D: /* Left Ctrl */
            kbd_state.ctrl_pressed = true;
            return;
        case 0x38: /* Left Alt */
            kbd_state.alt_pressed = true;
            return;
        case 0x3A: /* CapsLock toggle */
            kbd_state.capslock_on = !kbd_state.capslock_on;
            return;
    }

    /* --- Özel tuşlar --- */
    char c = 0;

    switch (scancode) {
        case 0x01: c = KEY_ESCAPE;    break;
        case 0x0E: c = KEY_BACKSPACE; break;
        case 0x0F: c = KEY_TAB;       break;
        case 0x1C: c = KEY_ENTER;     break;
        default:
            if (scancode < 128) {
                bool use_shift = kbd_state.shift_pressed;

                /* CapsLock sadece harfleri etkiler */
                if (kbd_state.capslock_on) {
                    char normal = scancode_to_char[scancode];
                    if (normal >= 'a' && normal <= 'z') {
                        use_shift = !use_shift;
                    }
                }

                if (use_shift) {
                    c = scancode_to_char_shift[scancode];
                } else {
                    c = scancode_to_char[scancode];
                }
            }
            break;
    }

    /* Callback varsa çağır */
    if (c && kbd_callback) {
        kbd_callback(c);
    }
}

void keyboard_init(void)
{
    /* Klavye tamponunu temizle */
    while (inb(KBD_STATUS_PORT) & 0x01) {
        inb(KBD_DATA_PORT);
    }

    irq_register_handler(IRQ_KEYBOARD, keyboard_handler);
    pic_clear_mask(IRQ_KEYBOARD);
}
