#ifndef WINTAKOS_KEYBOARD_H
#define WINTAKOS_KEYBOARD_H

#include "../include/types.h"

/* Ozel tus kodlari (ASCII disinda) */
#define KEY_NONE        0
#define KEY_ESCAPE      0x1B
#define KEY_BACKSPACE   0x08
#define KEY_TAB         0x09
#define KEY_ENTER       0x0A
#define KEY_LCTRL       0x80
#define KEY_LSHIFT      0x81
#define KEY_RSHIFT      0x82
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
#define KEY_NUMLOCK     0x9B
#define KEY_SCROLLLOCK  0x9C

/* Modifier durumlari */
typedef struct {
    uint8_t shift    : 1;
    uint8_t ctrl     : 1;
    uint8_t alt      : 1;
    uint8_t capslock : 1;
    uint8_t numlock  : 1;
} key_modifiers_t;

/* Klavye olayı */
typedef struct {
    char    ascii;          /* ASCII karakter (0 = ozel tus) */
    uint8_t keycode;        /* Ozel tus kodu */
    uint8_t scancode;       /* Ham scancode */
    uint8_t released;       /* 1 = tus birakildi */
    key_modifiers_t mods;   /* Modifier durumlari */
} key_event_t;

void keyboard_init(void);

/* Karakter tamponundan oku (0 = bos) */
char keyboard_getchar(void);

/* Son tus olayini al */
bool keyboard_poll(key_event_t* event);

/* Modifier durumlarini al */
key_modifiers_t keyboard_get_modifiers(void);

#endif
