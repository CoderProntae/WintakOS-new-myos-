#include "desktop.h"
#include "window.h"
#include "cursor.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/mouse.h"
#include "../cpu/pit.h"
#include "../cpu/rtc.h"
#include "../apps/setup.h"
#include "../lib/string.h"
#include "../include/version.h"

static uint32_t screen_w = 800;
static uint32_t screen_h = 600;

static uint8_t  last_sec_rtc = 0xFF;
static uint8_t  prev_buttons = 0;
static bool     start_menu_visible = false;
static uint32_t last_update_tick = 0;
static uint8_t  cursor_mode = CURSOR_NORMAL;
static uint32_t busy_end_tick = 0;

/* Cift tiklama algilama */
static uint32_t last_click_tick = 0;
static int32_t  last_click_x = -100;
static int32_t  last_click_y = -100;
#define DBLCLICK_TIME   30   /* ~300ms (100Hz PIT) */
#define DBLCLICK_DIST   8

/* Ikon secim ve surukleme */
static int32_t  selected_icon = -1;
static uint32_t select_anim_tick = 0;
static bool     icon_dragging = false;
static int32_t  drag_icon_idx = -1;
static int32_t  drag_icon_ox = 0;
static int32_t  drag_icon_oy = 0;

static uint32_t theme_bg_top;
static uint32_t theme_bg_bot;
static uint32_t theme_taskbar;
static uint32_t theme_taskbar_text;
static uint32_t theme_btn;

/* ---- 16x16 Ikonlar ---- */

static const uint8_t icon_terminal[16][16] = {
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {1,2,0,0,0,0,0,0,0,0,0,0,0,0,2,1},
    {1,2,0,3,3,0,0,0,0,0,0,0,0,0,2,1},
    {1,2,0,0,3,3,0,0,0,0,0,0,0,0,2,1},
    {1,2,0,0,0,3,3,0,0,0,0,0,0,0,2,1},
    {1,2,0,0,3,3,0,0,0,0,0,0,0,0,2,1},
    {1,2,0,3,3,0,0,0,0,0,0,0,0,0,2,1},
    {1,2,0,0,0,0,0,3,3,3,3,0,0,0,2,1},
    {1,2,0,0,0,0,0,0,0,0,0,0,0,0,2,1},
    {1,2,0,0,0,0,0,0,0,0,0,0,0,0,2,1},
    {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
    {0,1,2,2,2,2,2,2,2,2,2,2,2,2,1,0},
    {0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static const uint8_t icon_folder[16][16] = {
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,4,4,4,4,4,0,0,0,0,0,0,0,0,0,0},
    {4,5,5,5,5,5,4,4,4,4,4,4,4,4,0,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {4,5,5,5,5,5,5,5,5,5,5,5,5,5,4,0},
    {0,4,4,4,4,4,4,4,4,4,4,4,4,4,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
};

static const uint8_t icon_settings[16][16] = {
    {0,0,0,0,0,6,6,0,0,6,6,0,0,0,0,0},
    {0,0,0,0,6,7,7,6,6,7,7,6,0,0,0,0},
    {0,0,0,6,7,7,7,7,7,7,7,7,6,0,0,0},
    {0,0,6,7,7,7,0,0,0,0,7,7,7,6,0,0},
    {0,6,7,7,7,0,0,0,0,0,0,7,7,7,6,0},
    {6,7,7,7,0,0,0,0,0,0,0,0,7,7,7,6},
    {6,7,7,0,0,0,0,0,0,0,0,0,0,7,7,6},
    {0,6,7,0,0,0,0,0,0,0,0,0,0,7,6,0},
    {0,6,7,0,0,0,0,0,0,0,0,0,0,7,6,0},
    {6,7,7,0,0,0,0,0,0,0,0,0,0,7,7,6},
    {6,7,7,7,0,0,0,0,0,0,0,0,7,7,7,6},
    {0,6,7,7,7,0,0,0,0,0,0,7,7,7,6,0},
    {0,0,6,7,7,7,0,0,0,0,7,7,7,6,0,0},
    {0,0,0,6,7,7,7,7,7,7,7,7,6,0,0,0},
    {0,0,0,0,6,7,7,6,6,7,7,6,0,0,0,0},
    {0,0,0,0,0,6,6,0,0,6,6,0,0,0,0,0},
};

static const uint32_t icon_palette[] = {
    0x00000000,
    RGB(40, 40, 50),
    RGB(60, 60, 80),
    RGB(0, 255, 0),
    RGB(200, 170, 50),
    RGB(240, 210, 80),
    RGB(120, 120, 140),
    RGB(180, 180, 200),
};

typedef struct {
    const char* label;
    const uint8_t (*bitmap)[16];
    int     app_id;
    int32_t x, y;       /* suruklenebilir pozisyon */
} desktop_icon_t;

#define DESKTOP_ICON_COUNT 3
static desktop_icon_t desktop_icons[DESKTOP_ICON_COUNT];

#define ICON_GRID_X   30
#define ICON_GRID_Y   60
#define ICON_SPACING  70

static void icons_init(void)
{
    desktop_icons[0].label  = "Terminal";
    desktop_icons[0].bitmap = icon_terminal;
    desktop_icons[0].app_id = 0;
    desktop_icons[0].x = ICON_GRID_X;
    desktop_icons[0].y = ICON_GRID_Y;

    desktop_icons[1].label  = "Dosyalar";
    desktop_icons[1].bitmap = icon_folder;
    desktop_icons[1].app_id = 4;
    desktop_icons[1].x = ICON_GRID_X;
    desktop_icons[1].y = ICON_GRID_Y + ICON_SPACING;

    desktop_icons[2].label  = "Ayarlar";
    desktop_icons[2].bitmap = icon_settings;
    desktop_icons[2].app_id = 2;
    desktop_icons[2].x = ICON_GRID_X;
    desktop_icons[2].y = ICON_GRID_Y + ICON_SPACING * 2;
}

static void draw_icon_16(uint32_t px, uint32_t py, const uint8_t bmp[16][16],
                          uint32_t bg_color)
{
    for (uint32_t y = 0; y < 16; y++)
        for (uint32_t x = 0; x < 16; x++) {
            uint8_t idx = bmp[y][x];
            fb_put_pixel(px + x, py + y,
                         idx == 0 ? bg_color :
                         (idx < 8 ? icon_palette[idx] : bg_color));
        }
}

/* ---- Tema ---- */

static void apply_theme(uint8_t theme)
{
    switch (theme) {
        case 1:
            theme_bg_top = RGB(200, 215, 240);
            theme_bg_bot = RGB(230, 240, 250);
            theme_taskbar = RGB(230, 230, 238);
            theme_taskbar_text = RGB(30, 30, 40);
            theme_btn = RGB(90, 130, 200);
            break;
        case 2:
            theme_bg_top = RGB(5, 30, 80);
            theme_bg_bot = RGB(10, 80, 140);
            theme_taskbar = RGB(5, 20, 60);
            theme_taskbar_text = RGB(180, 220, 255);
            theme_btn = RGB(15, 60, 120);
            break;
        case 3:
            theme_bg_top = RGB(15, 40, 20);
            theme_bg_bot = RGB(30, 80, 40);
            theme_taskbar = RGB(20, 30, 15);
            theme_taskbar_text = RGB(200, 240, 200);
            theme_btn = RGB(30, 80, 40);
            break;
        case 4:
            theme_bg_top = RGB(40, 20, 60);
            theme_bg_bot = RGB(180, 80, 40);
            theme_taskbar = RGB(50, 20, 30);
            theme_taskbar_text = RGB(255, 220, 180);
            theme_btn = RGB(160, 60, 40);
            break;
        default:
            theme_bg_top = RGB(15, 20, 50);
            theme_bg_bot = RGB(30, 50, 100);
            theme_taskbar = RGB(20, 20, 35);
            theme_taskbar_text = COLOR_WHITE;
            theme_btn = RGB(45, 80, 160);
            break;
    }
}

/* ---- Cizim yardimcilari ---- */

static void draw_char_px(uint32_t px, uint32_t py, uint8_t c,
                          uint32_t fg, uint32_t bg)
{
    if (px >= screen_w - 8 || py >= screen_h - 16) return;
    const uint8_t* glyph = font8x16_data[c];
    for (uint32_t y = 0; y < 16; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < 8; x++)
            fb_put_pixel(px + x, py + y, (line & (0x80 >> x)) ? fg : bg);
    }
}

static void draw_string_px(uint32_t px, uint32_t py, const char* str,
                            uint32_t fg, uint32_t bg)
{
    while (*str) {
        if (px >= screen_w - 8) break;
        draw_char_px(px, py, (uint8_t)*str, fg, bg);
        px += 8;
        str++;
    }
}

static uint32_t gradient_color_at(uint32_t y, uint32_t height)
{
    if (height == 0) height = 1;
    uint32_t ratio = y * 255 / height;
    uint8_t r = (uint8_t)(((theme_bg_top >> 16) & 0xFF) * (255 - ratio) / 255 +
                           ((theme_bg_bot >> 16) & 0xFF) * ratio / 255);
    uint8_t g = (uint8_t)(((theme_bg_top >> 8) & 0xFF) * (255 - ratio) / 255 +
                           ((theme_bg_bot >> 8) & 0xFF) * ratio / 255);
    uint8_t b = (uint8_t)((theme_bg_top & 0xFF) * (255 - ratio) / 255 +
                           (theme_bg_bot & 0xFF) * ratio / 255);
    return RGB(r, g, b);
}

/* ---- Arka plan + Ikonlar ---- */

static void draw_background(void)
{
    uint32_t bg_h = screen_h - TASKBAR_HEIGHT;
    for (uint32_t y = 0; y < bg_h; y++) {
        uint32_t color = gradient_color_at(y, bg_h);
        for (uint32_t x = 0; x < screen_w; x++)
            fb_put_pixel(x, y, color);
    }

    /* Ikonlar */
    for (uint32_t i = 0; i < DESKTOP_ICON_COUNT; i++) {
        /* Surukleniyorsa bu ikonu atla — sonra cizilecek */
        if (icon_dragging && drag_icon_idx == (int32_t)i) continue;

        int32_t ix = desktop_icons[i].x;
        int32_t iy = desktop_icons[i].y;
        if (ix < 0 || iy < 0) continue;
        uint32_t ibg = gradient_color_at((uint32_t)iy, bg_h);

        /* Secim cercevesi */
        if (selected_icon == (int32_t)i) {
            uint32_t elapsed = pit_get_ticks() - select_anim_tick;
            uint32_t alpha = elapsed < 10 ? elapsed * 25 : 255;
            (void)alpha;
            /* Mavi seçim kutusu */
            uint32_t sel_color = RGB(60, 100, 200);
            fb_fill_rect((uint32_t)ix - 4, (uint32_t)iy - 4, 24, 44, sel_color);
            ibg = sel_color;
        }

        draw_icon_16((uint32_t)ix, (uint32_t)iy,
                     desktop_icons[i].bitmap, ibg);

        uint32_t label_len = strlen(desktop_icons[i].label);
        uint32_t lx = (uint32_t)ix + 8 - (label_len * 8) / 2;
        if (lx > screen_w) lx = 0;

        uint32_t label_bg;
        if (selected_icon == (int32_t)i)
            label_bg = RGB(60, 100, 200);
        else
            label_bg = gradient_color_at((uint32_t)iy + 20, bg_h);

        draw_string_px(lx, (uint32_t)iy + 20, desktop_icons[i].label,
                       COLOR_WHITE, label_bg);
    }

    /* Suruklenen ikonu mouse pozisyonunda ciz */
    if (icon_dragging && drag_icon_idx >= 0 &&
        drag_icon_idx < (int32_t)DESKTOP_ICON_COUNT) {
        mouse_state_t ms = mouse_get_state();
        int32_t dx = ms.x - drag_icon_ox;
        int32_t dy = ms.y - drag_icon_oy;
        if (dx >= 0 && dy >= 0 && dx < (int32_t)screen_w &&
            dy < (int32_t)(screen_h - TASKBAR_HEIGHT)) {
            uint32_t dbg = gradient_color_at((uint32_t)dy, bg_h);
            draw_icon_16((uint32_t)dx, (uint32_t)dy,
                         desktop_icons[drag_icon_idx].bitmap, dbg);
        }
    }

    /* Karsilama metni */
    setup_config_t* cfg = setup_get_config();
    uint32_t text_bg = gradient_color_at(30, bg_h);

    if (cfg->completed && strlen(cfg->username) > 0) {
        char msg[48];
        uint32_t p = 0;
        const char* prefix = "Ho\x03 geldin, ";
        while (*prefix) msg[p++] = *prefix++;
        const char* u = cfg->username;
        while (*u) msg[p++] = *u++;
        msg[p] = '\0';
        draw_string_px(screen_w / 2 - (p * 8) / 2, 30, msg,
                       RGB(220, 220, 255), text_bg);
    } else {
        draw_string_px(screen_w / 2 - 72, 30, WINTAKOS_FULL,
                       RGB(200, 200, 255), text_bg);
    }
}

/* ---- Taskbar ---- */

static void draw_taskbar(void)
{
    uint32_t ty = screen_h - TASKBAR_HEIGHT;
    fb_fill_rect(0, ty, screen_w, TASKBAR_HEIGHT, theme_taskbar);
    fb_draw_hline(0, ty, screen_w, RGB(60, 60, 100));

    uint32_t btn_c;
    if (start_menu_visible) {
        uint8_t br = (uint8_t)((theme_btn >> 16) & 0xFF);
        uint8_t bg = (uint8_t)((theme_btn >> 8) & 0xFF);
        uint8_t bb = (uint8_t)(theme_btn & 0xFF);
        if (br < 224) br += 32; else br = 255;
        if (bg < 224) bg += 32; else bg = 255;
        if (bb < 224) bb += 32; else bb = 255;
        btn_c = RGB(br, bg, bb);
    } else {
        btn_c = theme_btn;
    }

    fb_fill_rect(4, ty + 4, 80, TASKBAR_HEIGHT - 8, btn_c);
    draw_string_px(12, ty + 8, "Ba\x03lat", theme_taskbar_text, btn_c);

    /* Pencere tablari */
    uint32_t tab_x = 100;
    uint32_t wc = wm_get_count();
    for (uint32_t i = 0; i < wc; i++) {
        window_t* w = wm_get_window(i);
        if (!w || !(w->flags & WIN_FLAG_VISIBLE)) continue;

        bool is_min = (w->flags & WIN_FLAG_MINIMIZED) != 0;
        uint32_t tab_bg = is_min ? RGB(50, 50, 70) :
                          (w->active ? RGB(60, 100, 180) : RGB(40, 40, 60));

        if (tab_x + 100 < screen_w - 180) {
            fb_fill_rect(tab_x, ty + 4, 96, TASKBAR_HEIGHT - 8, tab_bg);
            fb_draw_rect_outline(tab_x, ty + 4, 96, TASKBAR_HEIGHT - 8,
                                 RGB(80, 80, 120));

            char short_title[12];
            uint32_t tl;
            for (tl = 0; tl < 10 && w->title[tl]; tl++)
                short_title[tl] = w->title[tl];
            short_title[tl] = '\0';

            draw_string_px(tab_x + 4, ty + 8, short_title,
                           theme_taskbar_text, tab_bg);
            tab_x += 100;
        }
    }

    /* Saat */
    rtc_time_t t;
    rtc_read(&t);
    char ts[9];
    ts[0] = '0' + (char)(t.hour / 10);
    ts[1] = '0' + (char)(t.hour % 10); ts[2] = ':';
    ts[3] = '0' + (char)(t.minute / 10);
    ts[4] = '0' + (char)(t.minute % 10); ts[5] = ':';
    ts[6] = '0' + (char)(t.second / 10);
    ts[7] = '0' + (char)(t.second % 10); ts[8] = '\0';
    draw_string_px(screen_w - 80, ty + 8, ts,
                   theme_taskbar_text, theme_taskbar);

    /* Tarih */
    char ds[11];
    ds[0] = '0' + (char)(t.day / 10);
    ds[1] = '0' + (char)(t.day % 10); ds[2] = '/';
    ds[3] = '0' + (char)(t.month / 10);
    ds[4] = '0' + (char)(t.month % 10); ds[5] = '/';
    ds[6] = '0' + (char)((t.year / 1000) % 10);
    ds[7] = '0' + (char)((t.year / 100) % 10);
    ds[8] = '0' + (char)((t.year / 10) % 10);
    ds[9] = '0' + (char)(t.year % 10); ds[10] = '\0';
    draw_string_px(screen_w - 168, ty + 8, ds,
                   RGB(160, 160, 180), theme_taskbar);
}

/* ---- Start Menu ---- */

#define START_MENU_W 160
#define START_MENU_H 168

static void draw_start_menu(void)
{
    if (!start_menu_visible) return;
    uint32_t mx = 4;
    uint32_t my = screen_h - TASKBAR_HEIGHT - START_MENU_H;
    uint32_t mbg = RGB(35, 35, 55);

    fb_fill_rect(mx, my, START_MENU_W, START_MENU_H, mbg);
    fb_draw_rect_outline(mx, my, START_MENU_W, START_MENU_H,
                         RGB(60, 60, 100));

    fb_fill_rect(mx + 1, my + 1, START_MENU_W - 2, 24, RGB(40, 80, 170));
    draw_string_px(mx + 8, my + 5, "WintakOS " WINTAKOS_VERSION,
                   COLOR_WHITE, RGB(40, 80, 170));

    const char* items[] = {
        "Terminal", "Sistem Bilgisi", "Ayarlar",
        "Hesap Makinesi", "Dosya Y\x08neticisi"
    };
    for (uint32_t i = 0; i < 5; i++) {
        uint32_t iy = my + 28 + i * 28;
        draw_string_px(mx + 12, iy + 4, items[i], COLOR_WHITE, mbg);
        if (i < 4)
            fb_draw_hline(mx + 1, iy + 26, START_MENU_W - 2,
                          RGB(60, 60, 80));
    }
}

/* ---- Cursor cizimi ---- */

static void cursor_draw(int32_t cx, int32_t cy)
{
    if (cursor_mode == CURSOR_BUSY) {
        /* Kum saati */
        uint32_t frame = (pit_get_ticks() / 10) % BUSY_FRAMES;
        for (uint32_t y = 0; y < BUSY_SIZE; y++)
            for (uint32_t x = 0; x < BUSY_SIZE; x++) {
                int32_t px = cx + (int32_t)x;
                int32_t py = cy + (int32_t)y;
                if (px < 0 || py < 0 ||
                    px >= (int32_t)screen_w || py >= (int32_t)screen_h)
                    continue;
                uint8_t v = busy_bitmap[frame][y][x];
                if (v == 1)
                    fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_BLACK);
                else if (v == 2)
                    fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_WHITE);
            }
    } else if (cursor_mode == CURSOR_DRAG) {
        /* Surukleme ok cursor */
        for (uint32_t y = 0; y < CURSOR_H; y++)
            for (uint32_t x = 0; x < CURSOR_W; x++) {
                int32_t px = cx + (int32_t)x;
                int32_t py = cy + (int32_t)y;
                if (px < 0 || py < 0 ||
                    px >= (int32_t)screen_w || py >= (int32_t)screen_h)
                    continue;
                uint8_t v = cursor_drag_bitmap[y][x];
                if (v == 1)
                    fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_BLACK);
                else if (v == 2)
                    fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_WHITE);
            }
    } else {
        /* Normal ok cursor */
        for (uint32_t y = 0; y < CURSOR_H; y++)
            for (uint32_t x = 0; x < CURSOR_W; x++) {
                int32_t px = cx + (int32_t)x;
                int32_t py = cy + (int32_t)y;
                if (px < 0 || py < 0 ||
                    px >= (int32_t)screen_w || py >= (int32_t)screen_h)
                    continue;
                uint8_t v = cursor_bitmap[y][x];
                if (v == 1)
                    fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_BLACK);
                else if (v == 2)
                    fb_put_pixel((uint32_t)px, (uint32_t)py, COLOR_WHITE);
            }
    }
}

/* ---- Ikon tiklama ---- */

static int icon_hit_test(int32_t mx, int32_t my)
{
    for (uint32_t i = 0; i < DESKTOP_ICON_COUNT; i++) {
        int32_t ix = desktop_icons[i].x;
        int32_t iy = desktop_icons[i].y;
        if (mx >= ix - 4 && mx < ix + 24 &&
            my >= iy - 4 && my < iy + 40)
            return (int)i;
    }
    return -1;
}

/* Public: tek tiklama — secim, app_id dondur */
int desktop_icon_click(int32_t mx, int32_t my)
{
    int hit = icon_hit_test(mx, my);
    if (hit >= 0) {
        selected_icon = hit;
        select_anim_tick = pit_get_ticks();
        wm_set_dirty();
    } else {
        /* Bos alana tiklanirsa secim kaldir */
        if (selected_icon >= 0) {
            selected_icon = -1;
            wm_set_dirty();
        }
    }
    return -1; /* tek tikta acma */
}

/* Public: cift tiklama — app_id dondur */
int desktop_icon_dblclick(int32_t mx, int32_t my)
{
    int hit = icon_hit_test(mx, my);
    if (hit >= 0) return desktop_icons[hit].app_id;
    return -1;
}

/* ---- Public API ---- */

void desktop_set_cursor(uint8_t mode)
{
    cursor_mode = mode;
    if (mode == CURSOR_BUSY) {
        busy_end_tick = pit_get_ticks() + 50; /* ~500ms bekleme */
    }
}

uint8_t desktop_get_cursor(void)
{
    return cursor_mode;
}

void desktop_init(void)
{
    framebuffer_t* info = fb_get_info();
    screen_w = info->width;
    screen_h = info->height;

    setup_config_t* cfg = setup_get_config();
    apply_theme(cfg->completed ? cfg->theme : 0);

    icons_init();
    draw_background();
    draw_taskbar();
    fb_swap();
    wm_init();
    last_sec_rtc = 0xFF;
    prev_buttons = 0;
    start_menu_visible = false;
    last_update_tick = 0;
    cursor_mode = CURSOR_NORMAL;
    selected_icon = -1;
    icon_dragging = false;
    drag_icon_idx = -1;
}

void desktop_apply_config(void)
{
    setup_config_t* cfg = setup_get_config();
    apply_theme(cfg->theme);
    wm_set_dirty();
}

bool desktop_start_menu_open(void)
{
    return start_menu_visible;
}

int desktop_start_menu_click(int32_t mx, int32_t my)
{
    uint32_t smx = 4;
    uint32_t smy = screen_h - TASKBAR_HEIGHT - START_MENU_H;
    if (mx < (int32_t)smx || mx >= (int32_t)(smx + START_MENU_W)) return -1;
    if (my < (int32_t)smy || my >= (int32_t)(smy + START_MENU_H)) return -1;
    int32_t ry = my - (int32_t)smy;
    if (ry >= 28 && ry < 56)  return 0;
    if (ry >= 56 && ry < 84)  return 1;
    if (ry >= 84 && ry < 112) return 2;
    if (ry >= 112 && ry < 140) return 3;
    if (ry >= 140 && ry < 168) return 4;
    return -1;
}

void desktop_update(void)
{
    uint32_t now = pit_get_ticks();
    if (now - last_update_tick < 2) return;
    last_update_tick = now;

    /* Busy cursor otomatik sona erme */
    if (cursor_mode == CURSOR_BUSY && now >= busy_end_tick) {
        cursor_mode = CURSOR_NORMAL;
        wm_set_dirty();
    }

    mouse_state_t ms = mouse_get_state();
    bool left_down = (ms.buttons & MOUSE_BTN_LEFT) != 0;
    bool left_pressed = left_down && !(prev_buttons & MOUSE_BTN_LEFT);
    bool left_released = !left_down && (prev_buttons & MOUSE_BTN_LEFT);

    /* --- Ikon surukleme mantigi --- */
    if (icon_dragging && left_down) {
        /* Surukle */
        cursor_mode = CURSOR_DRAG;
        wm_set_dirty();
    }

    if (icon_dragging && left_released) {
        /* Birak — yeni pozisyona yerles */
        if (drag_icon_idx >= 0 && drag_icon_idx < (int32_t)DESKTOP_ICON_COUNT) {
            int32_t nx = ms.x - drag_icon_ox;
            int32_t ny = ms.y - drag_icon_oy;
            /* Sinirlar */
            if (nx < 4) nx = 4;
            if (ny < 4) ny = 4;
            if (nx > (int32_t)screen_w - 24) nx = (int32_t)screen_w - 24;
            if (ny > (int32_t)screen_h - TASKBAR_HEIGHT - 40)
                ny = (int32_t)screen_h - TASKBAR_HEIGHT - 40;
            desktop_icons[drag_icon_idx].x = nx;
            desktop_icons[drag_icon_idx].y = ny;
        }
        icon_dragging = false;
        drag_icon_idx = -1;
        cursor_mode = CURSOR_NORMAL;
        wm_set_dirty();
    }

    /* --- Tiklama mantigi --- */
    if (left_pressed) {
        uint32_t ty = screen_h - TASKBAR_HEIGHT;

        /* Baslat butonu */
        if (ms.x >= 4 && ms.x < 84 &&
            ms.y >= (int32_t)ty + 4 && ms.y < (int32_t)ty + 28) {
            start_menu_visible = !start_menu_visible;
            wm_set_dirty();
        }
        /* Taskbar pencere tablari */
        else if (ms.y >= (int32_t)ty && ms.x >= 100 &&
                 ms.x < (int32_t)screen_w - 180) {
            uint32_t tab_idx = ((uint32_t)ms.x - 100) / 100;
            uint32_t visible_idx = 0;
            uint32_t wc = wm_get_count();
            for (uint32_t wi = 0; wi < wc; wi++) {
                window_t* w = wm_get_window(wi);
                if (!w || !(w->flags & WIN_FLAG_VISIBLE)) continue;
                if (visible_idx == tab_idx) {
                    if (w->flags & WIN_FLAG_MINIMIZED)
                        wm_restore_window(w);
                    break;
                }
                visible_idx++;
            }
            wm_set_dirty();
        }
        /* Start menu tikla */
        else if (start_menu_visible) {
            start_menu_visible = false;
            wm_set_dirty();
        }
        /* Masaustu alani */
        else if (ms.y < (int32_t)ty) {
            /* Cift tik kontrolu */
            bool is_dblclick = false;
            if (now - last_click_tick < DBLCLICK_TIME) {
                int32_t dx = ms.x - last_click_x;
                int32_t dy = ms.y - last_click_y;
                if (dx < 0) dx = -dx;
                if (dy < 0) dy = -dy;
                if (dx < DBLCLICK_DIST && dy < DBLCLICK_DIST)
                    is_dblclick = true;
            }

            int hit = icon_hit_test(ms.x, ms.y);

            if (is_dblclick && hit >= 0) {
                /* Cift tik — ikonu ac, kernel yakalayacak */
                last_click_tick = 0;
            } else {
                /* Tek tik — sec veya surukleme baslat */
                if (hit >= 0) {
                    selected_icon = hit;
                    select_anim_tick = now;
                    /* Surukleme hazirligi */
                    drag_icon_idx = hit;
                    drag_icon_ox = ms.x - desktop_icons[hit].x;
                    drag_icon_oy = ms.y - desktop_icons[hit].y;
                    /* Surukleme 5 tick sonra baslasin (drag threshold) */
                } else {
                    selected_icon = -1;
                    drag_icon_idx = -1;
                }
                wm_set_dirty();
            }

            last_click_tick = now;
            last_click_x = ms.x;
            last_click_y = ms.y;
        }
    }

    /* Surukleme threshold — 5 tick basili tutarsa surukleme baslar */
    if (left_down && drag_icon_idx >= 0 && !icon_dragging) {
        if (now - last_click_tick > 5) {
            icon_dragging = true;
            cursor_mode = CURSOR_DRAG;
            wm_set_dirty();
        }
    }

    wm_handle_mouse(ms.x, ms.y, ms.buttons);
    prev_buttons = ms.buttons;

    /* --- Render --- */
    bool need_swap = false;

    if (wm_is_dirty()) {
        draw_background();
        wm_draw_all();
        draw_taskbar();
        if (start_menu_visible) draw_start_menu();
        cursor_draw(ms.x, ms.y);
        wm_clear_dirty();
        need_swap = true;
    } else {
        rtc_time_t t;
        rtc_read(&t);
        if (t.second != last_sec_rtc) {
            last_sec_rtc = t.second;
            draw_background();
            wm_draw_all();
            draw_taskbar();
            if (start_menu_visible) draw_start_menu();
            cursor_draw(ms.x, ms.y);
            need_swap = true;
        } else {
            static int32_t prev_mx = -1, prev_my = -1;
            if (ms.x != prev_mx || ms.y != prev_my ||
                cursor_mode == CURSOR_BUSY) {
                draw_background();
                wm_draw_all();
                draw_taskbar();
                if (start_menu_visible) draw_start_menu();
                cursor_draw(ms.x, ms.y);
                need_swap = true;
                prev_mx = ms.x;
                prev_my = ms.y;
            }
        }
    }

    if (need_swap) fb_swap();
}
