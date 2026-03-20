#ifndef WINTAKOS_PIANO_H
#define WINTAKOS_PIANO_H

#include "../include/types.h"
#include "../gui/window.h"

typedef struct {
    window_t* win;
    int8_t    active_key;   /* -1 = yok, 0-12 = tus indeksi */
    uint8_t   octave;       /* 4 veya 5 */
} piano_t;

piano_t* piano_create(int32_t x, int32_t y);
void piano_input_char(piano_t* p, uint8_t c);
void piano_key_release(piano_t* p);

#endif
