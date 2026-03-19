#include "notepad.h"
#include "../gui/widget.h"
#include "../gui/window.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/keyboard.h"
#include "../cpu/pit.h"
#include "../lib/string.h"

static notepad_t notepads[4];
static uint32_t np_count = 0;

static void draw_char_at(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
{
    framebuffer_t* fb = fb_get_info();
    if (px >= fb->width - 8 || py >= fb->height - 16) return;
    const uint8_t* glyph = font8x16_data[c];
    for (uint32_t y = 0; y < 16; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < 8; x++)
            fb_put_pixel(px + x, py + y, (line & (0x80 >> x)) ? fg : bg);
    }
}

static void np_draw(window_t* win)
{
    notepad_t* np = NULL;
    for (uint32_t i = 0; i < np_count; i++)
        if (notepads[i].win == win) { np = &notepads[i]; break; }
    if (!np) return;

    int32_t bx = win->x + 4, by = win->y + 4;
    if (bx < 0 || by < 0) return;

    uint32_t row = 0, col = 0;
    for (uint32_t i = 0; i <= np->length; i++) {
        uint32_t px = (uint32_t)bx + col * 8;
        uint32_t py = (uint32_t)by + row * 16;

        if (i == np->cursor) {
            uint32_t blink = (pit_get_ticks() / 25) % 2;
            if (i < np->length) {
                draw_char_at(px, py, (uint8_t)np->text[i],
                             blink ? RGB(15, 15, 30) : RGB(30, 30, 30),
                             blink ? RGB(100, 200, 100) : COLOR_WHITE);
            } else {
                draw_char_at(px, py, blink ? '_' : ' ',
                             RGB(100, 200, 100), COLOR_WHITE);
            }
        } else if (i < np->length) {
            draw_char_at(px, py, (uint8_t)np->text[i], RGB(30, 30, 30), COLOR_WHITE);
        }

        if (i < np->length && np->text[i] == '\n') {
            row++; col = 0;
        } else {
            col++;
            if (col >= NOTEPAD_COLS) { col = 0; row++; }
        }
        if (row >= NOTEPAD_ROWS) break;
    }
}

void notepad_input_char(notepad_t* np, uint8_t c)
{
    if (!np || !np->active) return;

    if (c == '\b') {
        if (np->cursor > 0) {
            for (uint32_t i = np->cursor - 1; i < np->length - 1; i++)
                np->text[i] = np->text[i + 1];
            np->length--; np->cursor--;
            np->text[np->length] = '\0';
            wm_set_dirty();
        }
    }
    else if (c == '\n' || c >= 0x20 || (c >= 0x01 && c <= 0x10)) {
        if (np->length < NOTEPAD_MAX - 1) {
            for (uint32_t i = np->length; i > np->cursor; i--)
                np->text[i] = np->text[i - 1];
            np->text[np->cursor] = (char)c;
            np->length++; np->cursor++;
            np->text[np->length] = '\0';
            wm_set_dirty();
        }
    }
}

void notepad_input_key(notepad_t* np, uint8_t keycode)
{
    if (!np || !np->active) return;

    switch (keycode) {
        case KEY_LEFT:
            if (np->cursor > 0) { np->cursor--; wm_set_dirty(); }
            break;
        case KEY_RIGHT:
            if (np->cursor < np->length) { np->cursor++; wm_set_dirty(); }
            break;
        case KEY_HOME: np->cursor = 0; wm_set_dirty(); break;
        case KEY_END: np->cursor = np->length; wm_set_dirty(); break;
        case KEY_DELETE:
            if (np->cursor < np->length) {
                for (uint32_t i = np->cursor; i < np->length - 1; i++)
                    np->text[i] = np->text[i + 1];
                np->length--;
                np->text[np->length] = '\0';
                wm_set_dirty();
            }
            break;
        default: break;
    }
}

notepad_t* notepad_create(int32_t x, int32_t y)
{
    if (np_count >= 4) return NULL;
    notepad_t* np = &notepads[np_count++];
    memset(np->text, 0, sizeof(np->text));
    np->length = 0; np->cursor = 0; np->active = true;

    uint32_t w = NOTEPAD_COLS * 8 + 8;
    uint32_t h = NOTEPAD_ROWS * 16 + 8;

    np->win = wm_create_window(x, y, w, h, "Not Defteri", COLOR_WHITE);
    if (np->win) np->win->on_draw = np_draw;
    return np;
}
