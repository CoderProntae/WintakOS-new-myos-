#ifndef WINTAKOS_TERMINAL_H
#define WINTAKOS_TERMINAL_H

#include "../include/types.h"
#include "../gui/window.h"

#define TERM_COLS      80
#define TERM_ROWS      24
#define TERM_BUF_LINES 200
#define TERM_CMD_MAX   256

typedef struct {
    window_t* win;
    char      buffer[TERM_BUF_LINES][TERM_COLS + 1];
    uint32_t  buf_count;
    char      cmd[TERM_CMD_MAX];
    uint32_t  cmd_len;
    uint32_t  cmd_cursor;
    uint32_t  fg_color;
    uint32_t  bg_color;
    bool      active;
} terminal_t;

terminal_t* terminal_create(int32_t x, int32_t y);
void terminal_print(terminal_t* term, const char* str);
void terminal_print_color(terminal_t* term, const char* str, uint32_t color);
void terminal_input_char(terminal_t* term, uint8_t c);
void terminal_input_key(terminal_t* term, uint8_t keycode);

#endif
