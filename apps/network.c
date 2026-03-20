#include "network.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../net/net.h"
#include "../lib/string.h"
#include "../drivers/nic.h"

static network_app_t net_apps[2];
static uint32_t net_app_count = 0;

static void uint_to_str(uint32_t val, char* buf)
{
    if (val == 0) { buf[0]='0'; buf[1]=0; return; }
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

static void net_draw(window_t* win)
{
    net_status_t* st = net_get_status();
    char buf[48];

    widget_draw_label(win, 12, 8, "=== A\x05 Durumu ===", RGB(100, 200, 255));

    if (!st->nic_found) {
        widget_draw_label(win, 12, 36, "A\x05 kart\x01 bulunamad\x01!", RGB(255, 100, 100));
        widget_draw_label(win, 12, 56, "RTL8139 gerekli.", RGB(180, 180, 180));
        return;
    }

    /* NIC ismi */
    uint32_t bp = 0;
    const char* npf = "NIC: ";
    while (*npf) buf[bp++] = *npf++;
    const char* nn = nic_get_name();
    while (*nn) buf[bp++] = *nn++;
    buf[bp] = 0;
    widget_draw_label(win, 12, 36, buf, RGB(180, 220, 255));
    
    /* Durum */
    const char* link = st->link_up ? "Ba\x05l\x01" : "Ba\x05l\x01 De\x05il";
    uint32_t lcolor = st->link_up ? RGB(100, 255, 100) : RGB(255, 100, 100);
    widget_draw_label(win, 12, 58, "Durum: ", RGB(200, 200, 200));
    widget_draw_label(win, 68, 58, link, lcolor);

    /* MAC */
    mac_to_str(st->mac, buf);
    widget_draw_label(win, 12, 80, "MAC:", RGB(200, 200, 200));
    widget_draw_label(win, 52, 80, buf, RGB(180, 220, 255));

    /* IP */
    ip_to_str(st->ip, buf);
    widget_draw_label(win, 12, 102, "IP:", RGB(200, 200, 200));
    widget_draw_label(win, 44, 102, buf, RGB(180, 220, 255));

    /* Gateway */
    ip_to_str(st->gateway, buf);
    widget_draw_label(win, 12, 124, "GW:", RGB(200, 200, 200));
    widget_draw_label(win, 44, 124, buf, RGB(180, 180, 180));

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
    widget_draw_label(win, 12, 152, buf, RGB(180, 180, 180));

    /* Ping */
    if (st->ping_ms > 0) {
        p = 0;
        pf = "Ping: ";
        while (*pf) buf[p++] = *pf++;
        uint_to_str(st->ping_ms, nbuf);
        for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        pf = " ms";
        while (*pf) buf[p++] = *pf++;
        buf[p] = 0;
        widget_draw_label(win, 12, 174, buf, RGB(100, 255, 100));
    }

    /* Ping butonu */
    widget_draw_button(win, 12, 180, 100, 26, "Ping G\x0Cnder", RGB(50, 100, 180), COLOR_WHITE);
}

static void net_click(window_t* win, int32_t rx, int32_t ry)
{
    (void)win;
    net_status_t* st = net_get_status();

    if (widget_button_hit(win, 12, 180, 100, 26, rx, ry)) {
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

    app->win = wm_create_window(x, y, 260, 220, "A\x05 Durumu", RGB(30, 30, 48));
    if (app->win) {
        app->win->on_draw = net_draw;
        app->win->on_click = net_click;
    }
    return app;
}
