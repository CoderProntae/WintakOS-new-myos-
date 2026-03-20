#include "sysmonitor.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../drivers/speaker.h"
#include "../cpu/pit.h"
#include "../cpu/rtc.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"

static sysmonitor_t monitors[2];
static uint32_t mon_count = 0;

static void uint_to_str(uint32_t val, char* buf)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    uint32_t i = 0;
    while (tp > 0) buf[i++] = tmp[--tp];
    buf[i] = 0;
}

static void build_line(char* buf, const char* label, uint32_t val,
                        const char* unit)
{
    uint32_t p = 0;
    while (*label) buf[p++] = *label++;
    char nbuf[12];
    uint_to_str(val, nbuf);
    for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
    while (*unit) buf[p++] = *unit++;
    buf[p] = 0;
}

static void sysmon_draw(window_t* win)
{
    char buf[48];
    uint32_t total_mb = pmm_get_total_mb();
    uint32_t used_pages = pmm_get_used_pages();
    uint32_t free_pages = pmm_get_free_pages();
    uint32_t total_pages = pmm_get_total_pages();
    uint32_t heap_free_val = heap_get_free();
    uint32_t heap_used_val = heap_get_used();
    uint32_t uptime = pit_get_ticks() / PIT_FREQUENCY;
    uint32_t file_count = ramfs_file_count();

    /* Dinamik: progress bar genisligi pencere boyutuna gore */
    uint32_t bar_w = win->width - 24;
    if (bar_w > 400) bar_w = 400;
    if (bar_w < 100) bar_w = 100;

    uint32_t y = 8;
    widget_draw_label(win, 12, y, "=== Sistem Monit\x0Cr\x07 ===",
                      RGB(100, 200, 255));
    y += 24;

    widget_draw_label(win, 12, y, "Fiziksel Bellek:", RGB(200, 200, 200));
    y += 18;
    build_line(buf, "  Toplam: ", total_mb, " MB");
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
    y += 18;

    uint32_t used_pct = total_pages > 0 ?
                        (used_pages * 100) / total_pages : 0;
    widget_draw_progress(win, 12, y, bar_w, 14, used_pct,
                         RGB(60, 140, 220), RGB(40, 40, 60));
    y += 18;

    build_line(buf, "  Kullan\x01lan: ", used_pages, " sayfa");
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
    y += 18;
    build_line(buf, "  Bo\x03:       ", free_pages, " sayfa");
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
    y += 24;

    widget_draw_label(win, 12, y, "Kernel Heap:", RGB(200, 200, 200));
    y += 18;
    uint32_t heap_total = heap_free_val + heap_used_val;
    uint32_t heap_pct = heap_total > 0 ?
                        (heap_used_val * 100) / heap_total : 0;
    widget_draw_progress(win, 12, y, bar_w, 14, heap_pct,
                         RGB(200, 140, 40), RGB(40, 40, 60));
    y += 18;

    build_line(buf, "  Kullan\x01lan: ", heap_used_val, " byte");
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
    y += 18;
    build_line(buf, "  Bo\x03:       ", heap_free_val, " byte");
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
    y += 24;

    widget_draw_label(win, 12, y, "Sistem:", RGB(200, 200, 200));
    y += 18;

    uint32_t h = uptime / 3600;
    uint32_t m = (uptime % 3600) / 60;
    uint32_t s = uptime % 60;
    char tbuf[32]; uint32_t tp = 0;
    const char* pf = "  S\x07re: ";
    while (*pf) tbuf[tp++] = *pf++;
    tbuf[tp++] = '0' + (char)(h / 10);
    tbuf[tp++] = '0' + (char)(h % 10); tbuf[tp++] = ':';
    tbuf[tp++] = '0' + (char)(m / 10);
    tbuf[tp++] = '0' + (char)(m % 10); tbuf[tp++] = ':';
    tbuf[tp++] = '0' + (char)(s / 10);
    tbuf[tp++] = '0' + (char)(s % 10); tbuf[tp] = 0;
    widget_draw_label(win, 12, y, tbuf, RGB(180, 180, 180));
    y += 18;

    build_line(buf, "  Dosya: ", file_count, "");
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
    y += 18;

    uint32_t dp = 0;
    const char* sp = "  Ses: ";
    while (*sp) buf[dp++] = *sp++;
    const char* drv = sound_get_driver_name();
    while (*drv) buf[dp++] = *drv++;
    buf[dp] = 0;
    widget_draw_label(win, 12, y, buf, RGB(180, 180, 180));
}

sysmonitor_t* sysmonitor_create(int32_t x, int32_t y)
{
    if (mon_count >= 2) return NULL;
    sysmonitor_t* mon = &monitors[mon_count++];
    mon->last_tick = 0;
    mon->win = wm_create_window(x, y, 290, 290, "Sistem Monit\x0Cr\x07",
                                 RGB(30, 30, 48));
    if (mon->win) mon->win->on_draw = sysmon_draw;
    return mon;
}
