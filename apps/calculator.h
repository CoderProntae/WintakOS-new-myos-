#ifndef WINTAKOS_CALCULATOR_H
#define WINTAKOS_CALCULATOR_H

#include "../include/types.h"
#include "../gui/window.h"

typedef struct {
    window_t* win;
    char      display[20];
    int32_t   value1;
    int32_t   value2;
    char      op;
    bool      new_input;
} calculator_t;

calculator_t* calculator_create(int32_t x, int32_t y);
void calculator_input_char(calculator_t* calc, uint8_t c);

#endif
