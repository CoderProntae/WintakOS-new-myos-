#include "calculator.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../lib/string.h"

#define CALC_ROWS 5
#define CALC_COLS 4
#define BTN_PAD  4
#define BASE_BTN_W 48
#define BASE_BTN_H 32
#define BASE_DISP_H 50

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
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
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

static void px_draw_char(uint32_t px, uint32_t py, uint8_t c,
                          uint32_t fg, uint32_t bg)
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

static void px_draw_str(uint32_t px, uint32_t py, const char* s,
                         uint32_t fg, uint32_t bg)
{
    while (*s) {
        px_draw_char(px, py, (uint8_t)*s, fg, bg);
        px += 8;
        s++;
    }
}

/* Dinamik boyut hesapla */
static void calc_get_sizes(window_t* win, uint32_t* btn_w, uint32_t* btn_h,
                            uint32_t* disp_h)
{
    uint32_t avail_w = win->width - 16;
    uint32_t avail_h = win->height - 16;

    *btn_w = (avail_w - BTN_PAD * (CALC_COLS - 1)) / CALC_COLS;
    *disp_h = avail_h / 4;
    if (*disp_h < 40) *disp_h = 40;
    if (*disp_h > 100) *disp_h = 100;

    uint32_t remaining_h = avail_h - *disp_h - BTN_PAD;
    *btn_h = (remaining_h - BTN_PAD * (CALC_ROWS - 1)) / CALC_ROWS;
    if (*btn_h < 24) *btn_h = 24;
}

static void calc_draw(window_t* win)
{
    calculator_t* calc = NULL;
    for (uint32_t i = 0; i < calc_count; i++)
        if (calcs[i].win == win) { calc = &calcs[i]; break; }
    if (!calc) return;

    int32_t dx = win->x + 8, dy = win->y + 8;
    if (dx < 0 || dy < 0) return;

    uint32_t btn_w, btn_h, disp_h;
    calc_get_sizes(win, &btn_w, &btn_h, &disp_h);

    uint32_t disp_w = CALC_COLS * (btn_w + BTN_PAD);

    /* Display */
    fb_fill_rect((uint32_t)dx, (uint32_t)dy, disp_w, disp_h, RGB(25, 25, 45));
    fb_draw_rect_outline((uint32_t)dx, (uint32_t)dy, disp_w, disp_h,
                         RGB(80, 80, 120));

    /* Islem gosterimi */
    if (calc->op) {
        char expr[40]; uint32_t ep = 0;
        char v1buf[16];
        int_to_str(calc->value1, v1buf);
        for (uint32_t j = 0; v1buf[j]; j++) expr[ep++] = v1buf[j];
        expr[ep++] = ' '; expr[ep++] = calc->op; expr[ep++] = ' ';
        if (!calc->new_input) {
            for (uint32_t j = 0; calc->display[j]; j++) expr[ep++] = calc->display[j];
        }
        expr[ep] = '\0';
        uint32_t ex = (uint32_t)dx + disp_w - ep * 8 - 8;
        px_draw_str(ex, (uint32_t)dy + 4, expr,
                    RGB(150, 160, 200), RGB(25, 25, 45));
    }

    /* Mevcut deger */
    uint32_t dlen = strlen(calc->display);
    uint32_t tx = (uint32_t)dx + disp_w - dlen * 8 - 8;
    uint32_t text_y = (uint32_t)dy + disp_h / 2;
    px_draw_str(tx, text_y, calc->display, COLOR_WHITE, RGB(25, 25, 45));

    /* Butonlar */
    for (uint32_t r = 0; r < CALC_ROWS; r++) {
        for (uint32_t c = 0; c < CALC_COLS; c++) {
            uint32_t bx = 8 + c * (btn_w + BTN_PAD);
            uint32_t by = 8 + disp_h + BTN_PAD + r * (btn_h + BTN_PAD);

            uint32_t bg;
            const char* lbl = btn_labels[r][c];

            if (c == 3) {
                if (calc->op && lbl[0] == calc->op)
                    bg = RGB(100, 140, 220);
                else
                    bg = RGB(60, 100, 180);
            }
            else if (r == 0 && c == 0) bg = RGB(180, 60, 60);
            else if (r == 4 && c == 3) bg = RGB(40, 150, 60);
            else bg = RGB(60, 60, 80);

            widget_draw_button(win, bx, by, btn_w, btn_h, lbl,
                              bg, COLOR_WHITE);
        }
    }
}

static void calc_click(window_t* win, int32_t rx, int32_t ry)
{
    calculator_t* calc = NULL;
    for (uint32_t i = 0; i < calc_count; i++)
        if (calcs[i].win == win) { calc = &calcs[i]; break; }
    if (!calc) return;

    uint32_t btn_w, btn_h, disp_h;
    calc_get_sizes(win, &btn_w, &btn_h, &disp_h);

    int32_t start_y = 8 + (int32_t)disp_h + (int32_t)BTN_PAD;
    if (ry < start_y || rx < 8) return;

    uint32_t col = (uint32_t)(rx - 8) / (btn_w + BTN_PAD);
    uint32_t row = (uint32_t)(ry - start_y) / (btn_h + BTN_PAD);
    if (col >= CALC_COLS || row >= CALC_ROWS) return;

    const char* label = btn_labels[row][col];

    if (label[0] >= '0' && label[0] <= '9') {
        if (calc->new_input) {
            calc->display[0] = label[0]; calc->display[1] = '\0';
            calc->new_input = false;
        } else {
            uint32_t len = strlen(calc->display);
            if (len < 18) {
                calc->display[len] = label[0];
                calc->display[len + 1] = '\0';
            }
        }
    }
    else if (label[0] == 'C') {
        calc->display[0] = '0'; calc->display[1] = '\0';
        calc->value1 = 0; calc->op = 0; calc->new_input = true;
    }
    else if (label[0] == '+' && label[1] == '-') {
        if (calc->display[0] == '-') {
            for (uint32_t i = 0; calc->display[i + 1]; i++)
                calc->display[i] = calc->display[i + 1];
        } else {
            uint32_t len = strlen(calc->display);
            for (uint32_t i = len + 1; i > 0; i--)
                calc->display[i] = calc->display[i - 1];
            calc->display[0] = '-';
        }
    }
    else if (label[0] == '=') {
        calc->value2 = str_to_int(calc->display);
        int32_t result = 0;
        switch (calc->op) {
            case '+': result = calc->value1 + calc->value2; break;
            case '-': result = calc->value1 - calc->value2; break;
            case '*': result = calc->value1 * calc->value2; break;
            case '/': result = calc->value2 != 0 ?
                      calc->value1 / calc->value2 : 0; break;
            default: result = calc->value2; break;
        }
        int_to_str(result, calc->display);
        calc->value1 = result;
        calc->op = 0; calc->new_input = true;
    }
    else if (label[0] == '+' || label[0] == '-' ||
             label[0] == '*' || label[0] == '/') {
        if (calc->op && !calc->new_input) {
            calc->value2 = str_to_int(calc->display);
            int32_t result = 0;
            switch (calc->op) {
                case '+': result = calc->value1 + calc->value2; break;
                case '-': result = calc->value1 - calc->value2; break;
                case '*': result = calc->value1 * calc->value2; break;
                case '/': result = calc->value2 != 0 ?
                          calc->value1 / calc->value2 : 0; break;
                default: result = calc->value1; break;
            }
            int_to_str(result, calc->display);
            calc->value1 = result;
        } else {
            calc->value1 = str_to_int(calc->display);
        }
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

    uint32_t w = CALC_COLS * (BASE_BTN_W + BTN_PAD) + 16;
    uint32_t h = BASE_DISP_H + BTN_PAD +
                 CALC_ROWS * (BASE_BTN_H + BTN_PAD) + 16;

    calc->win = wm_create_window(x, y, w, h, "Hesap Makinesi",
                                  RGB(45, 45, 65));
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
