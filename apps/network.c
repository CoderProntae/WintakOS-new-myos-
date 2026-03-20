#include "network.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../net/net.h"
#include "../lib/string.h"
#include "../drivers/nic.h"

static network_app_t net_apps[2];
static uint32_t net_app_count = 0;

#define NET_CONTENT_W  240
#define NET_CONTENT_H  210

static void uint_to_str(uint32_t val, char* buf)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    uint32_t i = 0;
    while (tp > 0) buf[i++] = tmp[--tp];
    buf[i] = 0;
}

static void mac_to_str(mac_addr_t mac, char* buf)
{
    const char hex[] = "0123456789ABCDEF";
    uint32_t p = 0;
    for (int i = 0; i < 6; i++) {
        buf[p++] = hex[mac.addr[i] >> 4];
        buf[p++] = hex[mac.addr[i] & 0x0F];
        if (i < 5) buf[p++] = ':';
    }
    buf[p] = 0;
}

static void ip_to_str(ip_addr_t ip, char* buf)
{
    uint32_t p = 0;
    for (int i = 0; i < 4; i++) {
        char nbuf[4];
        uint_to_str(ip.octets[i], nbuf);
        for (int j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        if (i < 3) buf[p++] = '.';
    }
    buf[p] = 0;
}

/* Ortalama offset hesapla */
static void net_get_offset(window_t* win, uint32_t* ox, uint32_t* oy)
{
    int32_t extra_w = (int32_t)win->width - (int32_t)NET_CONTENT_W - 20;
    int32_t extra_h = (int32_t)win->height - (int32_t)NET_CONTENT_H - 10;
    *ox = extra_w > 0 ? (uint32_t)extra_w / 2 : 0;
    *oy = extra_h > 0 ? (uint32_t)extra_h / 2 : 0;
}

static void net_draw(window_t* win)
{
    net_status_t* st = net_get_status();
    char buf[48];

    uint32_t ox, oy;
    net_get_offset(win, &ox, &oy);
    uint32_t base_x = 12 + ox;
    uint32_t y = 8 + oy;

    widget_draw_label(win, base_x, y, "=== A\x05 Durumu ===",
                      RGB(100, 200, 255));
    y += 28;

    if (!st->nic_found) {
        widget_draw_label(win, base_x, y, "A\x05 kart\x01 bulunamad\x01!",
                          RGB(255, 100, 100));
        y += 20;
        widget_draw_label(win, base_x, y, "RTL8139 gerekli.",
                          RGB(180, 180, 180));
        return;
    }

    /* NIC ismi */
    uint32_t bp = 0;
    const char* npf = "NIC: ";
    while (*npf) buf[bp++] = *npf++;
    const char* nn = nic_get_name();
    while (*nn) buf[bp++] = *nn++;
    buf[bp] = 0;
    widget_draw_label(win, base_x, y, buf, RGB(180, 220, 255));
    y += 22;

    /* Durum */
    const char* link = st->link_up ? "Ba\x05l\x01" : "Ba\x05l\x01 De\x05il";
    uint32_t lcolor = st->link_up ? RGB(100, 255, 100) : RGB(255, 100, 100);
    widget_draw_label(win, base_x, y, "Durum: ", RGB(200, 200, 200));
    widget_draw_label(win, base_x + 56, y, link, lcolor);
    y += 22;

    /* MAC */
    mac_to_str(st->mac, buf);
    widget_draw_label(win, base_x, y, "MAC:", RGB(200, 200, 200));
    widget_draw_label(win, base_x + 40, y, buf, RGB(180, 220, 255));
    y += 22;

    /* IP */
    ip_to_str(st->ip, buf);
    widget_draw_label(win, base_x, y, "IP:", RGB(200, 200, 200));
    widget_draw_label(win, base_x + 32, y, buf, RGB(180, 220, 255));
    y += 22;

    /* Gateway */
    ip_to_str(st->gateway, buf);
    widget_draw_label(win, base_x, y, "GW:", RGB(200, 200, 200));
    widget_draw_label(win, base_x + 32, y, buf, RGB(180, 180, 180));
    y += 28;

    /* Paket sayaclari */
    uint32_t p = 0;
    const char* pf = "TX: ";
    while (*pf) buf[p++] = *pf++;
    char nbuf[12];
    uint_to_str(st->packets_sent, nbuf);
    for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
    pf = "  RX: ";
    while (*pf) buf[p++] = *pf++;
    uint_to_str(st->packets_recv, nbuf);
    for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
    buf[p] = 0;
    widget_draw_label(win, base_x, y, buf, RGB(180, 180, 180));
    y += 22;

    /* Ping */
    if (st->ping_ms > 0) {
        p = 0; pf = "Ping: ";
        while (*pf) buf[p++] = *pf++;
        uint_to_str(st->ping_ms, nbuf);
        for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        pf = " ms";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
        widget_draw_label(win, base_x, y, buf, RGB(100, 255, 100));
    }

    /* Ping butonu */
    widget_draw_button(win, base_x, y + 6, 100, 26, "Ping G\x0Cnder",
                       RGB(50, 100, 180), COLOR_WHITE);
}

static void net_click(window_t* win, int32_t rx, int32_t ry)
{
    net_status_t* st = net_get_status();

    uint32_t ox, oy;
    net_get_offset(win, &ox, &oy);
    uint32_t btn_x = 12 + ox;
    uint32_t btn_y = 8 + oy + 28 + 22 * 6 + 6;

    if (widget_button_hit(win, btn_x, btn_y, 100, 26, rx, ry)) {
        if (st->nic_found) {
            net_send_ping(st->gateway);
            wm_set_dirty();
        }
    }
}

network_app_t* network_create(int32_t x, int32_t y)
{
    if (net_app_count >= 2) return NULL;
    network_app_t* app = &net_apps[net_app_count++];

    app->win = wm_create_window(x, y, 260, 220, "A\x05 Durumu",
                                 RGB(30, 30, 48));
    if (app->win) {
        app->win->on_draw = net_draw;
        app->win->on_click = net_click;
    }
    return app;
}
