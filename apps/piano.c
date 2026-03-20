#include "piano.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/speaker.h"
#include "../cpu/pit.h"
#include "../lib/string.h"

static piano_t pianos[2];
static uint32_t piano_count = 0;

#define WHITE_KEY_W  30
#define WHITE_KEY_H  120
#define BLACK_KEY_W  20
#define BLACK_KEY_H  75
#define NUM_WHITE    8
#define PIANO_W      (NUM_WHITE * WHITE_KEY_W + 16)
#define PIANO_H      (WHITE_KEY_H + 60)

/* Beyaz tuslar: C D E F G A B C */
static const uint16_t white_freqs_o4[] = {
    NOTE_C4, NOTE_D4, NOTE_E4, NOTE_F4, NOTE_G4, NOTE_A4, NOTE_B4, NOTE_C5
};
static const uint16_t white_freqs_o5[] = {
    NOTE_C5, NOTE_D5, NOTE_E5, NOTE_F5, NOTE_G5, NOTE_A5, NOTE_B5, NOTE_C6
};
static const char white_labels[] = "CDEFGABC";

/* Siyah tuslar: CS DS - FS GS AS */
static const uint16_t black_freqs_o4[] = {
    NOTE_CS4, NOTE_DS4, 0, NOTE_FS4, NOTE_GS4, NOTE_AS4, 0
};
static const uint16_t black_freqs_o5[] = {
    NOTE_CS5, NOTE_DS5, 0, NOTE_FS5, NOTE_GS5, NOTE_AS5, 0
};

/* Klavye haritalamasi: asdfghjk = beyaz, wetyuo = siyah */
static const char kb_white[] = "asdfghjk";
static const char kb_black[] = "wetyu";

static void px_char(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
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

static void px_str(uint32_t px, uint32_t py, const char* s, uint32_t fg, uint32_t bg)
{
    while (*s) { px_char(px, py, (uint8_t)*s, fg, bg); px += 8; s++; }
}

static void piano_draw(window_t* win)
{
    piano_t* p = NULL;
    for (uint32_t i = 0; i < piano_count; i++)
        if (pianos[i].win == win) { p = &pianos[i]; break; }
    if (!p) return;

    int32_t bx = win->x + 8;
    int32_t by = win->y + 8;
    if (bx < 0 || by < 0) return;

    /* Oktav gosterge */
    char obuf[16];
    uint32_t op = 0;
    const char* pf = "Oktav: ";
    while (*pf) obuf[op++] = *pf++;
    obuf[op++] = '0' + p->octave;
    obuf[op++] = ' ';
    obuf[op++] = '(';
    obuf[op++] = 'Z';
    obuf[op++] = '/';
    obuf[op++] = 'X';
    obuf[op++] = ')';
    obuf[op] = 0;
    px_str((uint32_t)bx, (uint32_t)by, obuf, RGB(200, 200, 220), win->bg_color);

    uint32_t key_y = (uint32_t)by + 24;

    /* Beyaz tuslar */
    for (uint32_t i = 0; i < NUM_WHITE; i++) {
        uint32_t kx = (uint32_t)bx + i * WHITE_KEY_W;
        bool active = (p->active_key >= 0 && (uint32_t)p->active_key == i);

        uint32_t kcolor = active ? RGB(200, 220, 255) : COLOR_WHITE;
        fb_fill_rect(kx, key_y, WHITE_KEY_W - 2, WHITE_KEY_H, kcolor);
        fb_fill_rect(kx, key_y, 1, WHITE_KEY_H, RGB(120, 120, 120));
        fb_fill_rect(kx + WHITE_KEY_W - 3, key_y, 1, WHITE_KEY_H, RGB(120, 120, 120));
        fb_fill_rect(kx, key_y + WHITE_KEY_H - 1, WHITE_KEY_W - 2, 1, RGB(80, 80, 80));

        /* Nota adi */
        px_char(kx + 10, key_y + WHITE_KEY_H - 20, white_labels[i],
                RGB(80, 80, 80), kcolor);

        /* Klavye tusu */
        if (i < 8) {
            px_char(kx + 10, key_y + WHITE_KEY_H - 38, kb_white[i],
                    RGB(160, 160, 180), kcolor);
        }
    }

    /* Siyah tuslar */
    /* Siyah tus pozisyonlari: 0,1, (2 yok), 3,4,5, (6 yok) */
    uint32_t black_positions[] = {0, 1, 3, 4, 5};
    for (uint32_t i = 0; i < 5; i++) {
        uint32_t pos = black_positions[i];
        uint32_t kx = (uint32_t)bx + pos * WHITE_KEY_W + WHITE_KEY_W - BLACK_KEY_W / 2;

        bool active = (p->active_key >= 0 && (uint32_t)p->active_key == (NUM_WHITE + i));

        uint32_t kcolor = active ? RGB(60, 60, 100) : RGB(20, 20, 30);
        fb_fill_rect(kx, key_y, BLACK_KEY_W, BLACK_KEY_H, kcolor);
        fb_fill_rect(kx, key_y, 1, BLACK_KEY_H, RGB(40, 40, 50));
        fb_fill_rect(kx + BLACK_KEY_W - 1, key_y, 1, BLACK_KEY_H, RGB(40, 40, 50));

        /* Klavye tusu */
        if (i < 5) {
            px_char(kx + 6, key_y + BLACK_KEY_H - 20, kb_black[i],
                    RGB(120, 120, 140), kcolor);
        }
    }

    /* Kullanim bilgisi */
    uint32_t info_y = key_y + WHITE_KEY_H + 8;
    px_str((uint32_t)bx, info_y, "Klavye: asdfghjk wetyuo", RGB(140, 140, 160), win->bg_color);
}

static void piano_click(window_t* win, int32_t rx, int32_t ry)
{
    piano_t* p = NULL;
    for (uint32_t i = 0; i < piano_count; i++)
        if (pianos[i].win == win) { p = &pianos[i]; break; }
    if (!p) return;

    int32_t key_y_start = 32;
    if (ry < key_y_start || ry > key_y_start + (int32_t)WHITE_KEY_H) return;
    if (rx < 8) return;

    int32_t kx = rx - 8;
    const uint16_t* wf = (p->octave == 5) ? white_freqs_o5 : white_freqs_o4;
    const uint16_t* bf = (p->octave == 5) ? black_freqs_o5 : black_freqs_o4;

    /* Once siyah tuslari kontrol et (ustte oldugu icin oncelikli) */
    if (ry < key_y_start + (int32_t)BLACK_KEY_H) {
        uint32_t black_positions[] = {0, 1, 3, 4, 5};
        for (uint32_t i = 0; i < 5; i++) {
            uint32_t pos = black_positions[i];
            int32_t bkx = (int32_t)(pos * WHITE_KEY_W + WHITE_KEY_W - BLACK_KEY_W / 2);
            if (kx >= bkx && kx < bkx + (int32_t)BLACK_KEY_W) {
                uint32_t freq_idx = pos; /* black_freqs indeksi */
                if (freq_idx < 7 && bf[freq_idx] != 0) {
                    p->active_key = (int8_t)(NUM_WHITE + i);
                    speaker_play_tone(bf[freq_idx]);
                    wm_set_dirty();
                    return;
                }
            }
        }
    }

    /* Beyaz tus */
    uint32_t white_idx = (uint32_t)kx / WHITE_KEY_W;
    if (white_idx < NUM_WHITE) {
        p->active_key = (int8_t)white_idx;
        speaker_play_tone(wf[white_idx]);
        wm_set_dirty();
    }
}

void piano_input_char(piano_t* p, uint8_t c)
{
    if (!p) return;

    const uint16_t* wf = (p->octave == 5) ? white_freqs_o5 : white_freqs_o4;
    const uint16_t* bf = (p->octave == 5) ? black_freqs_o5 : black_freqs_o4;

    /* Oktav degistir */
    if (c == 'z' || c == 'Z') {
        if (p->octave > 4) { p->octave--; wm_set_dirty(); }
        return;
    }
    if (c == 'x' || c == 'X') {
        if (p->octave < 5) { p->octave++; wm_set_dirty(); }
        return;
    }

    /* Beyaz tuslar */
    for (uint32_t i = 0; i < 8; i++) {
        if (c == (uint8_t)kb_white[i]) {
            p->active_key = (int8_t)i;
            speaker_play_tone(wf[i]);
            wm_set_dirty();
            return;
        }
    }

    /* Siyah tuslar: w=CS, e=DS, t=FS, y=GS, u=AS */
    uint32_t black_freq_map[] = {0, 1, 3, 4, 5}; /* bf[] indeksleri */
    for (uint32_t i = 0; i < 5; i++) {
        if (c == (uint8_t)kb_black[i]) {
            uint32_t fi = black_freq_map[i];
            if (bf[fi] != 0) {
                p->active_key = (int8_t)(NUM_WHITE + i);
                speaker_play_tone(bf[fi]);
                wm_set_dirty();
            }
            return;
        }
    }
}

void piano_key_release(piano_t* p)
{
    if (!p) return;
    if (p->active_key >= 0) {
        p->active_key = -1;
        speaker_stop();
        wm_set_dirty();
    }
}

piano_t* piano_create(int32_t x, int32_t y)
{
    if (piano_count >= 2) return NULL;
    piano_t* p = &pianos[piano_count++];
    p->active_key = -1;
    p->octave = 4;

    p->win = wm_create_window(x, y, PIANO_W, PIANO_H, "Piyano", RGB(40, 40, 55));
    if (p->win) {
        p->win->on_draw = piano_draw;
        p->win->on_click = piano_click;
    }
    return p;
}
