#ifndef WINTAKOS_NOTEPAD_H
#define WINTAKOS_NOTEPAD_H

#include "../include/types.h"
#include "../gui/window.h"

#define NOTEPAD_COLS  32
#define NOTEPAD_ROWS  12
#define NOTEPAD_MAX   (NOTEPAD_COLS * NOTEPAD_ROWS)

typedef struct {
    window_t* win;
    char      text[NOTEPAD_MAX + 1];
    uint32_t  length;
    uint32_t  cursor;
    bool      active;
} notepad_t;

notepad_t* notepad_create(int32_t x, int32_t y);
void notepad_input_char(notepad_t* np, uint8_t c);
void notepad_input_key(notepad_t* np, uint8_t keycode);

#endif
