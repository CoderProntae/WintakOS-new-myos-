#include "filemanager.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"

static filemanager_t fms[2];
static uint32_t fm_count = 0;

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

static void uint_to_str_fm(uint32_t val, char* buf)
{
    if (val == 0) { buf[0]='0'; buf[1]=0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    uint32_t i = 0;
    while (tp > 0) buf[i++] = tmp[--tp];
    buf[i] = 0;
}

static void fm_draw(window_t* win)
{
    filemanager_t* fm = NULL;
    for (uint32_t i = 0; i < fm_count; i++)
        if (fms[i].win == win) { fm = &fms[i]; break; }
    if (!fm) return;

    int32_t bx = win->x + 4, by = win->y + 4;
    if (bx < 0 || by < 0) return;

    /* Header */
    fb_fill_rect((uint32_t)bx, (uint32_t)by, win->width - 8, 20, RGB(40, 70, 140));
    px_str((uint32_t)bx + 4, (uint32_t)by + 2, "Ad\x01", COLOR_WHITE, RGB(40, 70, 140));
    px_str((uint32_t)bx + 180, (uint32_t)by + 2, "Boyut", COLOR_WHITE, RGB(40, 70, 140));

    /* Dosya listesi */
    uint32_t row = 0;
    uint32_t max_rows = (win->height - 32) / 20;

    for (uint32_t fi = 0; fi < RAMFS_MAX_FILES && row < max_rows; fi++) {
        ramfs_file_t* f = ramfs_get_file(fi);
        if (!f) continue;

        uint32_t ry = (uint32_t)by + 24 + row * 20;

        /* Secili satir */
        uint32_t row_bg;
        if ((int32_t)fi == fm->selected)
            row_bg = RGB(50, 80, 160);
        else if (row % 2 == 0)
            row_bg = RGB(38, 38, 55);
        else
            row_bg = RGB(32, 32, 48);

        fb_fill_rect((uint32_t)bx, ry, win->width - 8, 20, row_bg);

        /* Dosya ikonu */
        uint32_t icon_c = f->is_dir ? RGB(255, 200, 80) : RGB(150, 200, 255);
        px_char((uint32_t)bx + 4, ry + 2, f->is_dir ? '/' : ' ', icon_c, row_bg);

        /* Dosya adi */
        px_str((uint32_t)bx + 16, ry + 2, f->name, COLOR_WHITE, row_bg);

        /* Boyut */
        char sbuf[12];
        uint_to_str_fm(f->size, sbuf);
        uint32_t slen = strlen(sbuf);
        px_str((uint32_t)bx + 180, ry + 2, sbuf, RGB(180, 180, 180), row_bg);
        px_char((uint32_t)bx + 180 + slen * 8, ry + 2, 'B', RGB(140, 140, 140), row_bg);

        row++;
    }

    /* Dosya sayisi */
    char cbuf[32];
    uint32_t cp = 0;
    char nbuf[12];
    uint_to_str_fm(ramfs_file_count(), nbuf);
    for (uint32_t j = 0; nbuf[j]; j++) cbuf[cp++] = nbuf[j];
    const char* suf = " dosya";
    while (*suf) cbuf[cp++] = *suf++;
    cbuf[cp] = 0;

    uint32_t footer_y = (uint32_t)by + (uint32_t)win->height - 24;
    fb_fill_rect((uint32_t)bx, footer_y, win->width - 8, 20, RGB(25, 25, 40));
    px_str((uint32_t)bx + 4, footer_y + 2, cbuf, RGB(140, 160, 200), RGB(25, 25, 40));
}

static void fm_click(window_t* win, int32_t rx, int32_t ry)
{
    filemanager_t* fm = NULL;
    for (uint32_t i = 0; i < fm_count; i++)
        if (fms[i].win == win) { fm = &fms[i]; break; }
    if (!fm) return;

    if (ry < 24) return;

    int32_t clicked_row = (ry - 24) / 20;
    int32_t actual_idx = -1;
    int32_t count = 0;

    for (uint32_t fi = 0; fi < RAMFS_MAX_FILES; fi++) {
        ramfs_file_t* f = ramfs_get_file(fi);
        if (!f) continue;
        if (count == clicked_row) { actual_idx = (int32_t)fi; break; }
        count++;
    }

    if (actual_idx >= 0) {
        fm->selected = actual_idx;
        wm_set_dirty();
    }

    (void)rx;
}

filemanager_t* filemanager_create(int32_t x, int32_t y)
{
    if (fm_count >= 2) return NULL;
    filemanager_t* fm = &fms[fm_count++];
    fm->selected = -1;
    fm->scroll = 0;
    fm->win = wm_create_window(x, y, 260, 280, "Dosya Y\x0Cneticisi", RGB(35, 35, 52));
    if (fm->win) {
        fm->win->on_draw = fm_draw;
        fm->win->on_click = fm_click;
    }
    return fm;
}
