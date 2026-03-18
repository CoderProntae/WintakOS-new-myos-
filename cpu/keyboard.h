#ifndef WINTAKOS_KEYBOARD_H
#define WINTAKOS_KEYBOARD_H

#include "../include/types.h"

/* Özel tuş kodları (ASCII dışı) */
#define KEY_NONE        0
#define KEY_ESCAPE      27
#define KEY_BACKSPACE   8
#define KEY_TAB         9
#define KEY_ENTER       10
#define KEY_LSHIFT      0x80
#define KEY_RSHIFT      0x81
#define KEY_LCTRL       0x82
#define KEY_LALT        0x83
#define KEY_CAPSLOCK    0x84
#define KEY_F1          0x85
#define KEY_F2          0x86
#define KEY_F3          0x87
#define KEY_F4          0x88
#define KEY_F5          0x89
#define KEY_F6          0x8A
#define KEY_F7          0x8B
#define KEY_F8          0x8C
#define KEY_F9          0x8D
#define KEY_F10         0x8E
#define KEY_F11         0x8F
#define KEY_F12         0x90
#define KEY_UP          0x91
#define KEY_DOWN        0x92
#define KEY_LEFT        0x93
#define KEY_RIGHT       0x94
#define KEY_HOME        0x95
#define KEY_END         0x96
#define KEY_PAGEUP      0x97
#define KEY_PAGEDOWN    0x98
#define KEY_INSERT      0x99
#define KEY_DELETE      0x9A

/* Klavye durumu */
typedef struct {
    bool shift_pressed;
    bool ctrl_pressed;
    bool alt_pressed;
    bool capslock_on;
} keyboard_state_t;

/* Klavye callback tipi */
typedef void (*keyboard_callback_t)(char c);

void keyboard_init(void);
void keyboard_set_callback(keyboard_callback_t cb);
keyboard_state_t keyboard_get_state(void);

#endif
