#include "calculator.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../lib/string.h"

#define CALC_ROWS 5
#define CALC_COLS 4
#define BTN_W    48
#define BTN_H    32
#define BTN_PAD  4
#define DISP_H   36

static calculator_t calcs[4];
static uint32_t calc_count = 0;

static const char* btn_labels[CALC_ROWS][CALC_COLS] = {
    {"C",  "(",  ")",  "/"},
    {"7",  "8",  "9",  "*"},
    {"4",  "5",  "6",  "-"},
    {"1",  "2",  "3",  "+"},
    {"0",  ".",  "+-", "="},
};

static void int_to_str(int32_t val, char* buf)
{
    if (val == 0) { buf[0]='0'; buf[1]=0; return; }
    uint32_t p = 0;
    bool neg = val < 0;
    if (neg) val = -val;
    char tmp[12]; uint32_t tp = 0;
    while (val > 0) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    if (neg) buf[p++] = '-';
    while (tp > 0) buf[p++] = tmp[--tp];
    buf[p] = 0;
}

static int32_t str_to_int(const char* s)
{
    int32_t val = 0; bool neg = false;
    if (*s == '-') { neg = true; s++; }
    while (*s >= '0' && *s <= '9') { val = val * 10 + (*s - '0'); s++; }
    return neg ? -val : val;
}

static void calc_draw(window_t* win)
{
    calculator_t* calc = NULL;
    for (uint32_t i = 0; i < calc_count; i++)
        if (calcs[i].win == win) { calc = &calcs[i]; break; }
    if (!calc) return;

    /* Display */
    int32_t dx = win->x + 8, dy = win->y + 8;
    if (dx > 0 && dy > 0) {
        fb_fill_rect((uint32_t)dx, (uint32_t)dy, CALC_COLS * (BTN_W + BTN_PAD), DISP_H, RGB(30, 30, 50));
        fb_fill_rect((uint32_t)dx, (uint32_t)dy, CALC_COLS * (BTN_W + BTN_PAD), 1, RGB(80, 80, 120));

        uint32_t len = strlen(calc->display);
        uint32_t tx = (uint32_t)dx + CALC_COLS * (BTN_W + BTN_PAD) - len * 8 - 8;
        const uint8_t* glyph;
        for (uint32_t i = 0; i < len; i++) {
            glyph = font8x16_data[(uint8_t)calc->display[i]];
            for (uint32_t gy = 0; gy < 16; gy++) {
                uint8_t line = glyph[gy];
                for (uint32_t gx = 0; gx < 8; gx++)
                    fb_put_pixel(tx + i*8 + gx, (uint32_t)dy + 10 + gy,
                        (line & (0x80 >> gx)) ? COLOR_WHITE : RGB(30, 30, 50));
            }
        }
    }

    /* Buttons */
    for (uint32_t r = 0; r < CALC_ROWS; r++) {
        for (uint32_t c = 0; c < CALC_COLS; c++) {
            uint32_t bx = 8 + c * (BTN_W + BTN_PAD);
            uint32_t by = 8 + DISP_H + BTN_PAD + r * (BTN_H + BTN_PAD);

            uint32_t bg;
            if (c == 3) bg = RGB(60, 100, 180);        /* Operator */
            else if (r == 0 && c == 0) bg = RGB(180, 60, 60);  /* C */
            else if (r == 4 && c == 3) bg = RGB(40, 150, 60);  /* = */
            else bg = RGB(60, 60, 80);

            widget_draw_button(win, bx, by, BTN_W, BTN_H,
                               btn_labels[r][c], bg, COLOR_WHITE);
        }
    }
}

static void calc_click(window_t* win, int32_t rx, int32_t ry)
{
    calculator_t* calc = NULL;
    for (uint32_t i = 0; i < calc_count; i++)
        if (calcs[i].win == win) { calc = &calcs[i]; break; }
    if (!calc) return;

    /* Hangi butona tiklandi? */
    int32_t start_y = 8 + (int32_t)DISP_H + (int32_t)BTN_PAD;
    if (ry < start_y || rx < 8) return;

    uint32_t col = (uint32_t)(rx - 8) / (BTN_W + BTN_PAD);
    uint32_t row = (uint32_t)(ry - start_y) / (BTN_H + BTN_PAD);
    if (col >= CALC_COLS || row >= CALC_ROWS) return;

    const char* label = btn_labels[row][col];

    if (label[0] >= '0' && label[0] <= '9') {
        if (calc->new_input) {
            calc->display[0] = label[0];
            calc->display[1] = '\0';
            calc->new_input = false;
        } else {
            uint32_t len = strlen(calc->display);
            if (len < 18) {
                calc->display[len] = label[0];
                calc->display[len+1] = '\0';
            }
        }
    }
    else if (label[0] == 'C') {
        calc->display[0] = '0'; calc->display[1] = '\0';
        calc->value1 = 0; calc->op = 0; calc->new_input = true;
    }
    else if (label[0] == '+' && label[1] == '-') {
        if (calc->display[0] == '-') {
            for (uint32_t i = 0; calc->display[i+1]; i++)
                calc->display[i] = calc->display[i+1];
        } else {
            uint32_t len = strlen(calc->display);
            for (uint32_t i = len + 1; i > 0; i--)
                calc->display[i] = calc->display[i-1];
            calc->display[0] = '-';
        }
    }
    else if (label[0] == '=' ) {
        calc->value2 = str_to_int(calc->display);
        int32_t result = 0;
        switch (calc->op) {
            case '+': result = calc->value1 + calc->value2; break;
            case '-': result = calc->value1 - calc->value2; break;
            case '*': result = calc->value1 * calc->value2; break;
            case '/': result = calc->value2 != 0 ? calc->value1 / calc->value2 : 0; break;
            default: result = calc->value2; break;
        }
        int_to_str(result, calc->display);
        calc->op = 0; calc->new_input = true;
    }
    else if (label[0]=='+' || label[0]=='-' || label[0]=='*' || label[0]=='/') {
        calc->value1 = str_to_int(calc->display);
        calc->op = label[0];
        calc->new_input = true;
    }

    wm_set_dirty();
}

calculator_t* calculator_create(int32_t x, int32_t y)
{
    if (calc_count >= 4) return NULL;
    calculator_t* calc = &calcs[calc_count++];
    calc->display[0] = '0'; calc->display[1] = '\0';
    calc->value1 = 0; calc->value2 = 0; calc->op = 0;
    calc->new_input = true;

    uint32_t w = CALC_COLS * (BTN_W + BTN_PAD) + 16;
    uint32_t h = DISP_H + BTN_PAD + CALC_ROWS * (BTN_H + BTN_PAD) + 16;

    calc->win = wm_create_window(x, y, w, h, "Hesap Makinesi", RGB(45, 45, 65));
    if (calc->win) {
        calc->win->on_draw = calc_draw;
        calc->win->on_click = calc_click;
    }
    return calc;
}

void calculator_input_char(calculator_t* calc, uint8_t c)
{
    (void)calc; (void)c;
}
