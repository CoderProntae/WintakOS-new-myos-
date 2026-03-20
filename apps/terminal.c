#include "terminal.h"
#include "../gui/window.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../cpu/pit.h"
#include "../cpu/pci.h"
#include "../cpu/rtc.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"
#include "../drivers/keyboard.h"
#include "../include/version.h"
#include "../net/net.h"
#include "../drivers/ata.h"
#include "../fs/vfs.h"

static void term_draw(window_t* win);
static void term_execute(terminal_t* term);

#define MAX_TERMINALS 4
static terminal_t terminals[MAX_TERMINALS];
static uint32_t term_count = 0;

/* Scrollbar genisligi — icerik alani buna gore daraltilir */
#define TERM_SCROLLBAR_W 14

static inline uint8_t dbg_inb(uint16_t port)
{
    uint8_t ret;
    __asm__ volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void dbg_outb(uint16_t port, uint8_t val)
{
    __asm__ volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t dbg_inw(uint16_t port)
{
    uint16_t ret;
    __asm__ volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static void draw_char_at(uint32_t px, uint32_t py, uint8_t c,
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

static terminal_t* find_term(window_t* win)
{
    for (uint32_t i = 0; i < term_count; i++)
        if (terminals[i].win == win) return &terminals[i];
    return NULL;
}

static void term_update_scrollbar(terminal_t* term)
{
    if (!term || !term->win) return;
    uint32_t visible = (term->win->height - 8) / 16;
    if (visible == 0) visible = 1;
    uint32_t total = term->buf_count + 1; /* +1 komut satiri */
    if (total <= visible) {
        term->win->flags &= (uint8_t)~WIN_FLAG_HAS_SCROLL;
    } else {
        wm_setup_scrollbar(term->win, total, visible);
        /* Otomatik en alta kaydir */
        uint32_t max_s = total - visible;
        term->win->scroll_pos = max_s;
    }
}

static void term_draw(window_t* win)
{
    terminal_t* term = find_term(win);
    if (!term) return;

    int32_t bx = win->x + 4, by = win->y + 4;
    if (bx < 0 || by < 0) return;
    uint32_t px0 = (uint32_t)bx, py0 = (uint32_t)by;

    uint32_t visible = (win->height - 8) / 16;
    if (visible == 0) return;
    uint32_t display_rows = visible - 1; /* son satir komut icin */

    /* Scroll pozisyonuna gore baslangic satirini hesapla */
    uint32_t start;
    if (term->buf_count <= display_rows) {
        start = 0;
    } else {
        uint32_t max_start = term->buf_count - display_rows;
        if (win->flags & WIN_FLAG_HAS_SCROLL) {
            start = win->scroll_pos;
            if (start > max_start) start = max_start;
        } else {
            start = max_start;
        }
    }

    /* Icerik alani genisligi (scrollbar varsa daralt) */
    uint32_t content_w = win->width - 8;
    if (win->flags & WIN_FLAG_HAS_SCROLL) content_w -= TERM_SCROLLBAR_W;
    uint32_t max_cols = content_w / 8;
    if (max_cols > TERM_COLS) max_cols = TERM_COLS;

    for (uint32_t i = 0; i < display_rows && (start + i) < term->buf_count; i++) {
        const char* line = term->buffer[(start + i) % TERM_BUF_LINES];
        uint32_t px = px0, py = py0 + i * 16;
        for (uint32_t j = 0; line[j] && j < max_cols; j++) {
            draw_char_at(px, py, (uint8_t)line[j],
                         term->fg_color, term->bg_color);
            px += 8;
        }
        /* Satir sonunu temizle */
        while (px < px0 + max_cols * 8) {
            draw_char_at(px, py, ' ', term->fg_color, term->bg_color);
            px += 8;
        }
    }

    /* Komut satiri */
    uint32_t cmd_y = py0 + display_rows * 16;
    draw_char_at(px0, cmd_y, '>', RGB(100, 255, 100), term->bg_color);
    draw_char_at(px0 + 8, cmd_y, ' ', term->fg_color, term->bg_color);

    uint32_t cpx = px0 + 16;
    uint32_t cmd_max = max_cols > 3 ? max_cols - 3 : 0;
    for (uint32_t i = 0; i < term->cmd_len && i < cmd_max; i++) {
        if (i == term->cmd_cursor) {
            draw_char_at(cpx, cmd_y, (uint8_t)term->cmd[i],
                         term->bg_color, RGB(100, 255, 100));
        } else {
            draw_char_at(cpx, cmd_y, (uint8_t)term->cmd[i],
                         term->fg_color, term->bg_color);
        }
        cpx += 8;
    }

    /* Imlec sondaysa */
    if (term->cmd_cursor >= term->cmd_len) {
        uint32_t cursor_x = px0 + 16 + term->cmd_cursor * 8;
        uint32_t blink = (pit_get_ticks() / 25) % 2;
        if (blink && term->active)
            draw_char_at(cursor_x, cmd_y, '_',
                         RGB(100, 255, 100), term->bg_color);
        else
            draw_char_at(cursor_x, cmd_y, ' ',
                         term->fg_color, term->bg_color);
    }

    /* Kalan alani temizle */
    for (uint32_t i = term->cmd_len; i < cmd_max; i++) {
        if (i != term->cmd_cursor) {
            uint32_t cx = px0 + 16 + (i + 1) * 8;
            if (cx < px0 + max_cols * 8)
                draw_char_at(cx, cmd_y, ' ', term->fg_color, term->bg_color);
        }
    }
}

static void term_add_line(terminal_t* term, const char* line)
{
    uint32_t idx = term->buf_count % TERM_BUF_LINES;
    uint32_t i;
    for (i = 0; i < TERM_COLS && line[i]; i++)
        term->buffer[idx][i] = line[i];
    term->buffer[idx][i] = '\0';
    term->buf_count++;
    term_update_scrollbar(term);
}

void terminal_print(terminal_t* term, const char* str)
{
    char line[TERM_COLS + 1];
    uint32_t pos = 0;
    while (*str) {
        if (*str == '\n' || pos >= TERM_COLS) {
            line[pos] = '\0';
            term_add_line(term, line);
            pos = 0;
            if (*str == '\n') str++;
        } else {
            line[pos++] = *str++;
        }
    }
    if (pos > 0) {
        line[pos] = '\0';
        term_add_line(term, line);
    }
    wm_set_dirty();
}

void terminal_print_color(terminal_t* term, const char* str, uint32_t color)
{
    uint32_t old = term->fg_color;
    term->fg_color = color;
    terminal_print(term, str);
    term->fg_color = old;
}

static bool str_eq(const char* a, const char* b)
{
    while (*a && *b) { if (*a != *b) return false; a++; b++; }
    return *a == *b;
}

static bool str_starts(const char* str, const char* prefix)
{
    while (*prefix) {
        if (*str != *prefix) return false;
        str++; prefix++;
    }
    return true;
}

static void uint_to_str(uint32_t val, char* buf, uint32_t max)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0 && tp < 11) {
        tmp[tp++] = '0' + (val % 10);
        val /= 10;
    }
    uint32_t i = 0;
    while (tp > 0 && i < max - 1) buf[i++] = tmp[--tp];
    buf[i] = 0;
}

static void term_execute(terminal_t* term)
{
    char pl[TERM_COLS + 1];
    pl[0] = '>'; pl[1] = ' ';
    uint32_t i;
    for (i = 0; i < term->cmd_len && i < TERM_COLS - 2; i++)
        pl[i + 2] = term->cmd[i];
    pl[i + 2] = '\0';
    term_add_line(term, pl);

    if (term->cmd_len == 0) { /* bos */ }
    else if (str_eq(term->cmd, "help")) {
        terminal_print_color(term, "Komutlar:", RGB(100, 200, 255));
        terminal_print(term, "  help       Yard\x01m");
        terminal_print(term, "  clear      Ekran\x01 temizle");
        terminal_print(term, "  sysinfo    Sistem bilgileri");
        terminal_print(term, "  uptime     \x10" "al\x01\x03ma s\x07resi");
        terminal_print(term, "  time       Saat ve tarih");
        terminal_print(term, "  mem        Bellek durumu");
        terminal_print(term, "  ls         Dosya listesi");
        terminal_print(term, "  cat <ad>   Dosya oku");
        terminal_print(term, "  touch <ad> Dosya olu\x03tur");
        terminal_print(term, "  rm <ad>    Dosya sil");
        terminal_print(term, "  write <ad> <i\x0Ferik>");
        terminal_print(term, "  echo <..>  Metin yazd\x01r");
        terminal_print(term, "  ifconfig   A\x05 bilgileri");
        terminal_print(term, "  ping       Gateway'e ping");
        terminal_print(term, "  diskinfo   Disk bilgileri");
        terminal_print(term, "  lsblk      Blok cihazlar\x01");
        terminal_print(term, "  ver        S\x07r\x07m bilgisi");
    }
    else if (str_eq(term->cmd, "clear")) {
        term->buf_count = 0;
        memset(term->buffer, 0, sizeof(term->buffer));
        term_update_scrollbar(term);
    }
    else if (str_eq(term->cmd, "sysinfo")) {
        terminal_print_color(term, "=== Sistem ===", RGB(255, 200, 100));
        terminal_print(term, "OS:    " WINTAKOS_FULL);
        terminal_print(term, "CPU:   i386 Protected");
        char buf[40]; char nbuf[12];
        uint_to_str(pmm_get_total_mb(), nbuf, 12);
        uint32_t p = 0;
        const char* pf = "RAM:   ";
        while (*pf) buf[p++] = *pf++;
        for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        buf[p++] = ' '; buf[p++] = 'M'; buf[p++] = 'B'; buf[p] = 0;
        terminal_print(term, buf);
        terminal_print(term, "Video: 800x600x32 VESA");
        terminal_print(term, "Klavye: T\x07rk\x0Fe Q");
    }
    else if (str_eq(term->cmd, "uptime")) {
        uint32_t sec = pit_get_ticks() / PIT_FREQUENCY;
        char buf[16];
        buf[0] = '0' + (char)((sec / 3600) / 10);
        buf[1] = '0' + (char)((sec / 3600) % 10); buf[2] = ':';
        buf[3] = '0' + (char)(((sec % 3600) / 60) / 10);
        buf[4] = '0' + (char)(((sec % 3600) / 60) % 10); buf[5] = ':';
        buf[6] = '0' + (char)((sec % 60) / 10);
        buf[7] = '0' + (char)((sec % 60) % 10); buf[8] = 0;
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "time")) {
        rtc_time_t t; rtc_read(&t);
        char buf[20];
        buf[0] = '0' + (char)(t.hour / 10);
        buf[1] = '0' + (char)(t.hour % 10); buf[2] = ':';
        buf[3] = '0' + (char)(t.minute / 10);
        buf[4] = '0' + (char)(t.minute % 10); buf[5] = ':';
        buf[6] = '0' + (char)(t.second / 10);
        buf[7] = '0' + (char)(t.second % 10); buf[8] = ' ';
        buf[9] = '0' + (char)(t.day / 10);
        buf[10] = '0' + (char)(t.day % 10); buf[11] = '/';
        buf[12] = '0' + (char)(t.month / 10);
        buf[13] = '0' + (char)(t.month % 10); buf[14] = '/';
        buf[15] = '0' + (char)((t.year / 1000) % 10);
        buf[16] = '0' + (char)((t.year / 100) % 10);
        buf[17] = '0' + (char)((t.year / 10) % 10);
        buf[18] = '0' + (char)(t.year % 10); buf[19] = 0;
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "mem")) {
        char buf[48]; char nbuf[12];
        terminal_print_color(term, "=== Bellek ===", RGB(255, 200, 100));
        uint_to_str(pmm_get_total_mb(), nbuf, 12);
        uint32_t p = 0; const char* pf = "Toplam: ";
        while (*pf) buf[p++] = *pf++;
        for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        buf[p++] = ' '; buf[p++] = 'M'; buf[p++] = 'B'; buf[p] = 0;
        terminal_print(term, buf);
        uint_to_str(pmm_get_free_pages(), nbuf, 12);
        p = 0; pf = "Bo\x03:    ";
        while (*pf) buf[p++] = *pf++;
        for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        pf = " sayfa"; while (*pf) buf[p++] = *pf++; buf[p] = 0;
        terminal_print(term, buf);
        uint_to_str(heap_get_free(), nbuf, 12);
        p = 0; pf = "Heap:   ";
        while (*pf) buf[p++] = *pf++;
        for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        pf = " byte"; while (*pf) buf[p++] = *pf++; buf[p] = 0;
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "ls")) {
        terminal_print_color(term, "=== Dosyalar ===", RGB(100, 200, 255));
        char buf[48]; char nbuf[12];
        for (uint32_t fi = 0; fi < RAMFS_MAX_FILES; fi++) {
            ramfs_file_t* f = ramfs_get_file(fi);
            if (!f) continue;
            uint32_t p = 0;
            buf[p++] = ' '; buf[p++] = ' ';
            for (uint32_t j = 0; f->name[j] && p < 30; j++)
                buf[p++] = f->name[j];
            while (p < 22) buf[p++] = ' ';
            uint_to_str(f->size, nbuf, 12);
            for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
            buf[p++] = 'B'; buf[p] = 0;
            terminal_print(term, buf);
        }
    }
    else if (str_starts(term->cmd, "cat ")) {
        const char* fn = term->cmd + 4;
        if (!ramfs_exists(fn))
            terminal_print_color(term, "Dosya bulunamad\x01.",
                                 RGB(255, 100, 100));
        else {
            char fbuf[RAMFS_MAX_FILESIZE + 1];
            int rd = ramfs_read(fn, fbuf, RAMFS_MAX_FILESIZE);
            if (rd > 0) { fbuf[rd] = '\0'; terminal_print(term, fbuf); }
        }
    }
    else if (str_starts(term->cmd, "touch ")) {
        const char* fn = term->cmd + 6;
        if (ramfs_exists(fn))
            terminal_print(term, "Dosya zaten var.");
        else {
            int r = ramfs_create(fn, false);
            if (r >= 0)
                terminal_print_color(term, "Olu\x03turuldu.",
                                     RGB(100, 255, 100));
            else
                terminal_print_color(term, "Hata!", RGB(255, 100, 100));
        }
    }
    else if (str_starts(term->cmd, "rm ")) {
        if (ramfs_delete(term->cmd + 3))
            terminal_print_color(term, "Silindi.", RGB(100, 255, 100));
        else
            terminal_print_color(term, "Bulunamad\x01.", RGB(255, 100, 100));
    }
    else if (str_starts(term->cmd, "write ")) {
        const char* rest = term->cmd + 6;
        char fn[RAMFS_MAX_NAME]; uint32_t fi = 0;
        while (*rest && *rest != ' ' && fi < RAMFS_MAX_NAME - 1)
            fn[fi++] = *rest++;
        fn[fi] = '\0';
        if (*rest == ' ') rest++;
        if (!ramfs_exists(fn)) ramfs_create(fn, false);
        ramfs_write(fn, rest, strlen(rest));
        terminal_print_color(term, "Yaz\x01ld\x01.", RGB(100, 255, 100));
    }
    else if (str_eq(term->cmd, "ifconfig")) {
        net_status_t* ns = net_get_status();
        if (!ns->nic_found) {
            terminal_print_color(term, "A\x05 kart\x01 bulunamad\x01.",
                                 RGB(255, 100, 100));
        } else {
            char buf[48];
            terminal_print_color(term, "=== A\x05 Bilgisi ===",
                                 RGB(100, 200, 255));
            uint32_t p = 0;
            const char* pf = "MAC: ";
            while (*pf) buf[p++] = *pf++;
            const char hex[] = "0123456789ABCDEF";
            for (int mi = 0; mi < 6; mi++) {
                buf[p++] = hex[ns->mac.addr[mi] >> 4];
                buf[p++] = hex[ns->mac.addr[mi] & 0x0F];
                if (mi < 5) buf[p++] = ':';
            }
            buf[p] = 0;
            terminal_print(term, buf);

            p = 0; pf = "IP:  ";
            while (*pf) buf[p++] = *pf++;
            for (int ii = 0; ii < 4; ii++) {
                char nb[4];
                uint_to_str(ns->ip.octets[ii], nb, 4);
                for (int j = 0; nb[j]; j++) buf[p++] = nb[j];
                if (ii < 3) buf[p++] = '.';
            }
            buf[p] = 0;
            terminal_print(term, buf);

            p = 0; pf = "GW:  ";
            while (*pf) buf[p++] = *pf++;
            for (int gi = 0; gi < 4; gi++) {
                char nb[4];
                uint_to_str(ns->gateway.octets[gi], nb, 4);
                for (int j = 0; nb[j]; j++) buf[p++] = nb[j];
                if (gi < 3) buf[p++] = '.';
            }
            buf[p] = 0;
            terminal_print(term, buf);

            p = 0; pf = "TX:  ";
            while (*pf) buf[p++] = *pf++;
            char nb2[12];
            uint_to_str(ns->packets_sent, nb2, 12);
            for (uint32_t j = 0; nb2[j]; j++) buf[p++] = nb2[j];
            pf = "  RX: ";
            while (*pf) buf[p++] = *pf++;
            uint_to_str(ns->packets_recv, nb2, 12);
            for (uint32_t j = 0; nb2[j]; j++) buf[p++] = nb2[j];
            buf[p] = 0;
            terminal_print(term, buf);
        }
    }
    else if (str_eq(term->cmd, "ping")) {
        net_status_t* ns = net_get_status();
        if (!ns->nic_found) {
            terminal_print_color(term, "A\x05 kart\x01 yok.",
                                 RGB(255, 100, 100));
        } else {
            terminal_print(term, "Gateway'e ping g\x0Cnderiliyor...");
            net_send_ping(ns->gateway);
        }
    }
    else if (str_eq(term->cmd, "diskinfo")) {
        terminal_print_color(term, "=== Disk Debug ===", RGB(100, 200, 255));

        char dbuf[64];
        uint32_t dp;
        const char hex[] = "0123456789ABCDEF";

        /* Her kanal ve drive icin tek tek debug */
        uint16_t bases[2] = { 0x1F0, 0x170 };
        uint16_t ctrls[2] = { 0x3F6, 0x376 };
        const char* ch_names[2] = { "Primary", "Secondary" };

        for (int ch = 0; ch < 2; ch++) {
            for (int dr = 0; dr < 2; dr++) {
                dp = 0;
                /* Kanal ve drive adi */
                const char* cn = ch_names[ch];
                while (*cn) dbuf[dp++] = *cn++;
                dbuf[dp++] = ' ';
                const char* dn = dr == 0 ? "Master" : "Slave";
                while (*dn) dbuf[dp++] = *dn++;
                dbuf[dp++] = ':';
                dbuf[dp] = 0;
                terminal_print_color(term, dbuf, RGB(200, 200, 100));

                uint16_t base = bases[ch];
                uint16_t ctrl = ctrls[ch];
                uint8_t sel = (dr == 0) ? 0xA0 : 0xB0;
                uint8_t status;

                /* 1. Floating bus */
                status = ata_inb(base + 7);
                dp = 0;
                const char* pf = "  Status: 0x";
                while (*pf) dbuf[dp++] = *pf++;
                dbuf[dp++] = hex[status >> 4];
                dbuf[dp++] = hex[status & 0x0F];
                dbuf[dp] = 0;
                terminal_print(term, dbuf);

                if (status == 0xFF) {
                    terminal_print(term, "  -> Floating bus, atla");
                    continue;
                }

                /* 2. Drive sec */
                ata_outb(base + 6, sel);
                for (volatile uint32_t w = 0; w < 50000; w++);

                status = ata_inb(base + 7);
                dp = 0;
                pf = "  Secim sonrasi: 0x";
                while (*pf) dbuf[dp++] = *pf++;
                dbuf[dp++] = hex[status >> 4];
                dbuf[dp++] = hex[status & 0x0F];
                dbuf[dp] = 0;
                terminal_print(term, dbuf);

                /* 3. IDENTIFY gonder */
                ata_outb(base + 2, 0);
                ata_outb(base + 3, 0);
                ata_outb(base + 4, 0);
                ata_outb(base + 5, 0);
                ata_outb(base + 7, 0xEC);

                /* Kisa bekle */
                ata_inb(ctrl); ata_inb(ctrl);
                ata_inb(ctrl); ata_inb(ctrl);
                for (volatile uint32_t w = 0; w < 50000; w++);

                /* 4. IDENTIFY sonrasi status */
                status = ata_inb(base + 7);
                dp = 0;
                pf = "  IDENTIFY sonrasi: 0x";
                while (*pf) dbuf[dp++] = *pf++;
                dbuf[dp++] = hex[status >> 4];
                dbuf[dp++] = hex[status & 0x0F];
                dbuf[dp] = 0;
                terminal_print(term, dbuf);

                if (status == 0) {
                    terminal_print(term, "  -> Cevap yok");
                    continue;
                }

                /* 5. BSY bekle */
                uint32_t bsy_count = 0;
                for (uint32_t t = 0; t < 500000; t++) {
                    status = ata_inb(base + 7);
                    if (!(status & 0x80)) break;
                    bsy_count++;
                }

                dp = 0;
                pf = "  BSY sonrasi: 0x";
                while (*pf) dbuf[dp++] = *pf++;
                dbuf[dp++] = hex[status >> 4];
                dbuf[dp++] = hex[status & 0x0F];
                dbuf[dp] = 0;
                terminal_print(term, dbuf);

                /* 6. LBA mid/hi — device type */
                uint8_t lm = ata_inb(base + 4);
                uint8_t lh = ata_inb(base + 5);
                dp = 0;
                pf = "  LBA mid/hi: 0x";
                while (*pf) dbuf[dp++] = *pf++;
                dbuf[dp++] = hex[lm >> 4];
                dbuf[dp++] = hex[lm & 0x0F];
                dbuf[dp++] = '/';
                dbuf[dp++] = '0';
                dbuf[dp++] = 'x';
                dbuf[dp++] = hex[lh >> 4];
                dbuf[dp++] = hex[lh & 0x0F];
                dbuf[dp] = 0;
                terminal_print(term, dbuf);

                if (lm == 0x14 && lh == 0xEB) {
                    terminal_print_color(term, "  -> ATAPI cihaz",
                                         RGB(255, 200, 100));
                    continue;
                }
                if (lm == 0x69 && lh == 0x96) {
                    terminal_print_color(term, "  -> SATA ATAPI",
                                         RGB(255, 200, 100));
                    continue;
                }
                if (lm == 0x3C && lh == 0xC3) {
                    terminal_print_color(term, "  -> SATA ATA",
                                         RGB(100, 255, 100));
                }
                if (lm == 0 && lh == 0) {
                    terminal_print_color(term, "  -> ATA cihaz",
                                         RGB(100, 255, 100));
                }

                /* 7. ERR/DRQ kontrol */
                if (status & 0x01) {
                    terminal_print_color(term, "  -> ERR flag!",
                                         RGB(255, 100, 100));
                    continue;
                }

                if (status & 0x08) {
                    terminal_print_color(term, "  -> DRQ hazir, veri okunabilir",
                                         RGB(100, 255, 100));

                    /* Veriyi oku */
                    uint16_t ident[256];
                    for (int i = 0; i < 256; i++)
                        ident[i] = ata_inw(base + 0);

                    /* Model */
                    char model[41];
                    for (int i = 0; i < 20; i++) {
                        model[i*2]   = (char)(ident[27+i] >> 8);
                        model[i*2+1] = (char)(ident[27+i] & 0xFF);
                    }
                    model[40] = '\0';
                    for (int i = 39; i >= 0; i--) {
                        if (model[i]==' ' || model[i]=='\0')
                            model[i] = '\0';
                        else break;
                    }

                    dp = 0;
                    pf = "  Model: ";
                    while (*pf) dbuf[dp++] = *pf++;
                    for (int i = 0; model[i] && dp < 58; i++)
                        dbuf[dp++] = model[i];
                    dbuf[dp] = 0;
                    terminal_print_color(term, dbuf, RGB(100, 255, 100));

                    /* Sectors */
                    uint32_t secs = (uint32_t)ident[60] |
                                    ((uint32_t)ident[61] << 16);
                    uint32_t mb = secs / 2048;
                    dp = 0;
                    pf = "  Boyut: ";
                    while (*pf) dbuf[dp++] = *pf++;
                    char nbuf[12];
                    uint_to_str(mb, nbuf, 12);
                    for (uint32_t j = 0; nbuf[j]; j++) dbuf[dp++] = nbuf[j];
                    pf = " MB";
                    while (*pf) dbuf[dp++] = *pf++;
                    dbuf[dp] = 0;
                    terminal_print_color(term, dbuf, RGB(100, 255, 100));
                } else {
                    terminal_print_color(term, "  -> DRQ yok, veri alinamadi",
                                         RGB(255, 100, 100));
                }
            }
        }

        /* AHCI debug */
        terminal_print(term, "");
        pci_device_t pdev;
        if (pci_find_device(0x01, 0x06, &pdev)) {
            terminal_print_color(term, "AHCI controller bulundu",
                                 RGB(100, 255, 100));
        } else {
            terminal_print(term, "AHCI controller yok");
        }
    }
    else if (str_eq(term->cmd, "lsblk")) {
        terminal_print_color(term, "NAME    SIZE    TYPE",
                             RGB(100, 200, 255));
        for (uint8_t i = 0; i < ATA_MAX_DRIVES; i++) {
            ata_drive_t* d = ata_get_info(i);
            if (!d || !d->present) continue;
            char buf[48]; char nbuf[12];
            uint32_t p = 0;
            buf[p++] = 'd'; buf[p++] = 'i'; buf[p++] = 's';
            buf[p++] = 'k'; buf[p++] = '0' + i;
            while (p < 8) buf[p++] = ' ';
            if (d->is_ata) {
                uint_to_str(d->size_mb, nbuf, 12);
                for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
                buf[p++] = 'M';
            } else {
                buf[p++] = '-';
            }
            while (p < 16) buf[p++] = ' ';
            const char* tp = d->is_ata ? "ATA" : "ATAPI";
            while (*tp) buf[p++] = *tp++;
            buf[p] = 0;
            terminal_print(term, buf);
        }
        if (ata_get_drive_count() == 0) {
            terminal_print_color(term, "(disk yok)",
                                 RGB(150, 150, 150));
        }
    }
    else if (str_eq(term->cmd, "ver")) {
        terminal_print_color(term, WINTAKOS_FULL, RGB(100, 200, 255));
    }
    else if (str_starts(term->cmd, "echo ")) {
        terminal_print(term, term->cmd + 5);
    }
    else {
        terminal_print_color(term, "Bilinmeyen komut. 'help' yaz\x01n.",
                             RGB(255, 100, 100));
    }

    term->cmd_len = 0;
    term->cmd_cursor = 0;
    term->cmd[0] = '\0';
    wm_set_dirty();
}

void terminal_input_char(terminal_t* term, uint8_t c)
{
    if (!term || !term->active) return;

    if (c == '\n') { term_execute(term); return; }
    if (c == '\b') {
        if (term->cmd_cursor > 0) {
            for (uint32_t i = term->cmd_cursor - 1; i < term->cmd_len - 1; i++)
                term->cmd[i] = term->cmd[i + 1];
            term->cmd_len--;
            term->cmd_cursor--;
            term->cmd[term->cmd_len] = '\0';
            wm_set_dirty();
        }
        return;
    }
    if (c >= 0x20 || (c >= 0x01 && c <= 0x10)) {
        if (term->cmd_len < TERM_CMD_MAX - 1) {
            for (uint32_t i = term->cmd_len; i > term->cmd_cursor; i--)
                term->cmd[i] = term->cmd[i - 1];
            term->cmd[term->cmd_cursor] = (char)c;
            term->cmd_len++;
            term->cmd_cursor++;
            term->cmd[term->cmd_len] = '\0';
            wm_set_dirty();
        }
    }
}

void terminal_input_key(terminal_t* term, uint8_t keycode)
{
    if (!term || !term->active) return;

    switch (keycode) {
        case KEY_LEFT:
            if (term->cmd_cursor > 0) { term->cmd_cursor--; wm_set_dirty(); }
            break;
        case KEY_RIGHT:
            if (term->cmd_cursor < term->cmd_len) {
                term->cmd_cursor++;
                wm_set_dirty();
            }
            break;
        case KEY_HOME:
            term->cmd_cursor = 0;
            wm_set_dirty();
            break;
        case KEY_END:
            term->cmd_cursor = term->cmd_len;
            wm_set_dirty();
            break;
        case KEY_DELETE:
            if (term->cmd_cursor < term->cmd_len) {
                for (uint32_t i = term->cmd_cursor; i < term->cmd_len - 1; i++)
                    term->cmd[i] = term->cmd[i + 1];
                term->cmd_len--;
                term->cmd[term->cmd_len] = '\0';
                wm_set_dirty();
            }
            break;
        case KEY_PAGE_UP:
            wm_scroll_up(term->win, 5);
            break;
        case KEY_PAGE_DOWN:
            wm_scroll_down(term->win, 5);
            break;
        default: break;
    }
}

terminal_t* terminal_create(int32_t x, int32_t y)
{
    if (term_count >= MAX_TERMINALS) return NULL;
    terminal_t* term = &terminals[term_count++];
    memset(term->buffer, 0, sizeof(term->buffer));
    term->buf_count = 0;
    term->cmd_len = 0;
    term->cmd_cursor = 0;
    term->cmd[0] = '\0';
    term->fg_color = RGB(200, 200, 200);
    term->bg_color = RGB(15, 15, 25);
    term->active = true;
    term->win = wm_create_window(x, y, TERM_COLS * 8 + 8 + TERM_SCROLLBAR_W,
                                  TERM_ROWS * 16 + 8,
                                  "Terminal", term->bg_color);
    if (term->win) term->win->on_draw = term_draw;
    terminal_print_color(term, "WintakOS Terminal " WINTAKOS_VERSION,
                         RGB(100, 200, 255));
    terminal_print(term, "'help' yazarak ba\x03lay\x01n.");
    terminal_print(term, "");
    return term;
}
