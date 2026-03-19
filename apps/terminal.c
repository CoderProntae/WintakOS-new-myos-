#include "terminal.h"
#include "../gui/window.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../cpu/pit.h"
#include "../cpu/rtc.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../fs/ramfs.h"
#include "../lib/string.h"

static void term_draw(window_t* win);
static void term_execute(terminal_t* term);

#define MAX_TERMINALS 4
static terminal_t terminals[MAX_TERMINALS];
static uint32_t term_count = 0;

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

static void term_draw(window_t* win)
{
    terminal_t* term = NULL;
    for (uint32_t i = 0; i < term_count; i++)
        if (terminals[i].win == win) { term = &terminals[i]; break; }
    if (!term) return;

    int32_t bx = win->x + 4, by = win->y + 4;
    if (bx < 0 || by < 0) return;
    uint32_t px0 = (uint32_t)bx, py0 = (uint32_t)by;

    uint32_t visible = TERM_ROWS - 1;
    uint32_t start = term->buf_count > visible ? term->buf_count - visible : 0;

    for (uint32_t i = 0; i < visible && (start + i) < term->buf_count; i++) {
        const char* line = term->buffer[(start + i) % TERM_BUF_LINES];
        uint32_t px = px0, py = py0 + i * 16;
        for (uint32_t j = 0; line[j] && j < TERM_COLS; j++) {
            draw_char_at(px, py, (uint8_t)line[j], term->fg_color, term->bg_color);
            px += 8;
        }
    }

    uint32_t cmd_y = py0 + visible * 16;
    draw_char_at(px0, cmd_y, '>', RGB(100, 255, 100), term->bg_color);
    draw_char_at(px0 + 8, cmd_y, ' ', term->fg_color, term->bg_color);

    uint32_t cpx = px0 + 16;
    for (uint32_t i = 0; i < term->cmd_len && i < TERM_COLS - 3; i++) {
        uint32_t char_color = term->fg_color;
        if (i == term->cmd_cursor) {
            /* Imleç altindaki karakter ters renk */
            draw_char_at(cpx, cmd_y, (uint8_t)term->cmd[i], term->bg_color, RGB(100, 255, 100));
        } else {
            draw_char_at(cpx, cmd_y, (uint8_t)term->cmd[i], char_color, term->bg_color);
        }
        cpx += 8;
    }

    /* Imleç sondaysa yanip sonen blok */
    if (term->cmd_cursor >= term->cmd_len) {
        uint32_t cursor_x = px0 + 16 + term->cmd_cursor * 8;
        uint32_t blink = (pit_get_ticks() / 25) % 2;
        if (blink && term->active)
            draw_char_at(cursor_x, cmd_y, '_', RGB(100, 255, 100), term->bg_color);
        else
            draw_char_at(cursor_x, cmd_y, ' ', term->fg_color, term->bg_color);
    }
}

static void term_add_line(terminal_t* term, const char* line)
{
    uint32_t idx = term->buf_count % TERM_BUF_LINES;
    uint32_t i;
    for (i = 0; i < TERM_COLS && line[i]; i++) term->buffer[idx][i] = line[i];
    term->buffer[idx][i] = '\0';
    term->buf_count++;
}

void terminal_print(terminal_t* term, const char* str)
{
    char line[TERM_COLS + 1];
    uint32_t pos = 0;
    while (*str) {
        if (*str == '\n' || pos >= TERM_COLS) {
            line[pos] = '\0'; term_add_line(term, line); pos = 0;
            if (*str == '\n') str++;
        } else { line[pos++] = *str++; }
    }
    if (pos > 0) { line[pos] = '\0'; term_add_line(term, line); }
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
    while (*prefix) { if (*str != *prefix) return false; str++; prefix++; }
    return true;
}

static void uint_to_str(uint32_t val, char* buf, uint32_t max)
{
    if (val == 0) { buf[0] = '0'; buf[1] = 0; return; }
    char tmp[12]; uint32_t tp = 0;
    while (val > 0 && tp < 11) { tmp[tp++] = '0' + (val % 10); val /= 10; }
    uint32_t i = 0;
    while (tp > 0 && i < max - 1) buf[i++] = tmp[--tp];
    buf[i] = 0;
}

static void term_execute(terminal_t* term)
{
    char pl[TERM_COLS + 1];
    pl[0] = '>'; pl[1] = ' ';
    uint32_t i;
    for (i = 0; i < term->cmd_len && i < TERM_COLS - 2; i++) pl[i+2] = term->cmd[i];
    pl[i+2] = '\0';
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
        terminal_print(term, "  ver        S\x07r\x07m bilgisi");
    }
    else if (str_eq(term->cmd, "clear")) {
        term->buf_count = 0;
        memset(term->buffer, 0, sizeof(term->buffer));
    }
    else if (str_eq(term->cmd, "sysinfo")) {
        terminal_print_color(term, "=== Sistem ===", RGB(255, 200, 100));
        terminal_print(term, "OS:    WintakOS v0.9.0");
        terminal_print(term, "CPU:   i386 Protected");
        char buf[40]; char nbuf[12];
        uint_to_str(pmm_get_total_mb(), nbuf, 12);
        uint32_t p = 0;
        const char* pf = "RAM:   ";
        while (*pf) buf[p++] = *pf++;
        for (uint32_t j = 0; nbuf[j]; j++) buf[p++] = nbuf[j];
        buf[p++]=' '; buf[p++]='M'; buf[p++]='B'; buf[p]=0;
        terminal_print(term, buf);
        terminal_print(term, "Video: 800x600x32 VESA");
        terminal_print(term, "Klavye: T\x07rk\x0Fe Q");
    }
    else if (str_eq(term->cmd, "uptime")) {
        uint32_t sec = pit_get_ticks() / PIT_FREQUENCY;
        char buf[16];
        buf[0]='0'+(char)((sec/3600)/10); buf[1]='0'+(char)((sec/3600)%10); buf[2]=':';
        buf[3]='0'+(char)(((sec%3600)/60)/10); buf[4]='0'+(char)(((sec%3600)/60)%10); buf[5]=':';
        buf[6]='0'+(char)((sec%60)/10); buf[7]='0'+(char)((sec%60)%10); buf[8]=0;
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "time")) {
        rtc_time_t t; rtc_read(&t);
        char buf[20];
        buf[0]='0'+(char)(t.hour/10); buf[1]='0'+(char)(t.hour%10); buf[2]=':';
        buf[3]='0'+(char)(t.minute/10); buf[4]='0'+(char)(t.minute%10); buf[5]=':';
        buf[6]='0'+(char)(t.second/10); buf[7]='0'+(char)(t.second%10); buf[8]=' ';
        buf[9]='0'+(char)(t.day/10); buf[10]='0'+(char)(t.day%10); buf[11]='/';
        buf[12]='0'+(char)(t.month/10); buf[13]='0'+(char)(t.month%10); buf[14]='/';
        buf[15]='0'+(char)((t.year/1000)%10); buf[16]='0'+(char)((t.year/100)%10);
        buf[17]='0'+(char)((t.year/10)%10); buf[18]='0'+(char)(t.year%10); buf[19]=0;
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "mem")) {
        char buf[48]; char nbuf[12];
        terminal_print_color(term, "=== Bellek ===", RGB(255, 200, 100));
        uint_to_str(pmm_get_total_mb(), nbuf, 12);
        uint32_t p=0; const char*pf="Toplam: ";
        while(*pf) buf[p++]=*pf++;
        for(uint32_t j=0;nbuf[j];j++) buf[p++]=nbuf[j];
        buf[p++]=' ';buf[p++]='M';buf[p++]='B';buf[p]=0;
        terminal_print(term, buf);
        uint_to_str(pmm_get_free_pages(), nbuf, 12);
        p=0; pf="Bo\x03:    ";
        while(*pf) buf[p++]=*pf++;
        for(uint32_t j=0;nbuf[j];j++) buf[p++]=nbuf[j];
        pf=" sayfa"; while(*pf) buf[p++]=*pf++; buf[p]=0;
        terminal_print(term, buf);
        uint_to_str(heap_get_free(), nbuf, 12);
        p=0; pf="Heap:   ";
        while(*pf) buf[p++]=*pf++;
        for(uint32_t j=0;nbuf[j];j++) buf[p++]=nbuf[j];
        pf=" byte"; while(*pf) buf[p++]=*pf++; buf[p]=0;
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "ls")) {
        terminal_print_color(term, "=== Dosyalar ===", RGB(100, 200, 255));
        char buf[48]; char nbuf[12];
        for (uint32_t fi = 0; fi < RAMFS_MAX_FILES; fi++) {
            ramfs_file_t* f = ramfs_get_file(fi);
            if (!f) continue;
            uint32_t p = 0;
            buf[p++]=' '; buf[p++]=' ';
            for (uint32_t j=0; f->name[j] && p<30; j++) buf[p++]=f->name[j];
            while (p < 22) buf[p++]=' ';
            uint_to_str(f->size, nbuf, 12);
            for (uint32_t j=0; nbuf[j]; j++) buf[p++]=nbuf[j];
            buf[p++]='B'; buf[p]=0;
            terminal_print(term, buf);
        }
    }
    else if (str_starts(term->cmd, "cat ")) {
        const char* fn = term->cmd + 4;
        if (!ramfs_exists(fn)) terminal_print_color(term, "Dosya bulunamad\x01.", RGB(255,100,100));
        else {
            char fbuf[RAMFS_MAX_FILESIZE+1];
            int rd = ramfs_read(fn, fbuf, RAMFS_MAX_FILESIZE);
            if (rd > 0) { fbuf[rd]='\0'; terminal_print(term, fbuf); }
        }
    }
    else if (str_starts(term->cmd, "touch ")) {
        const char* fn = term->cmd + 6;
        if (ramfs_exists(fn)) terminal_print(term, "Dosya zaten var.");
        else {
            int r = ramfs_create(fn, false);
            if (r >= 0) terminal_print_color(term, "Olu\x03turuldu.", RGB(100,255,100));
            else terminal_print_color(term, "Hata!", RGB(255,100,100));
        }
    }
    else if (str_starts(term->cmd, "rm ")) {
        if (ramfs_delete(term->cmd + 3)) terminal_print_color(term, "Silindi.", RGB(100,255,100));
        else terminal_print_color(term, "Bulunamad\x01.", RGB(255,100,100));
    }
    else if (str_starts(term->cmd, "write ")) {
        const char* rest = term->cmd + 6;
        char fn[RAMFS_MAX_NAME]; uint32_t fi = 0;
        while (*rest && *rest != ' ' && fi < RAMFS_MAX_NAME-1) fn[fi++] = *rest++;
        fn[fi] = '\0';
        if (*rest == ' ') rest++;
        if (!ramfs_exists(fn)) ramfs_create(fn, false);
        ramfs_write(fn, rest, strlen(rest));
        terminal_print_color(term, "Yaz\x01ld\x01.", RGB(100,255,100));
    }
    else if (str_eq(term->cmd, "ver")) {
        terminal_print_color(term, "WintakOS v0.9.0", RGB(100, 200, 255));
        terminal_print(term, "Milestone 9");
    }
    else if (str_starts(term->cmd, "echo ")) { terminal_print(term, term->cmd + 5); }
    else { terminal_print_color(term, "Bilinmeyen komut. 'help' yaz\x01n.", RGB(255,100,100)); }

    term->cmd_len = 0; term->cmd_cursor = 0; term->cmd[0] = '\0';
    wm_set_dirty();
}

void terminal_input_char(terminal_t* term, uint8_t c)
{
    if (!term || !term->active) return;

    if (c == '\n') { term_execute(term); return; }
    if (c == '\b') {
        if (term->cmd_cursor > 0) {
            /* Imleç pozisyonundan sil */
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
            /* Imleç pozisyonuna ekle */
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
            if (term->cmd_cursor < term->cmd_len) { term->cmd_cursor++; wm_set_dirty(); }
            break;
        case KEY_HOME:
            term->cmd_cursor = 0; wm_set_dirty();
            break;
        case KEY_END:
            term->cmd_cursor = term->cmd_len; wm_set_dirty();
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
        default: break;
    }
}

terminal_t* terminal_create(int32_t x, int32_t y)
{
    if (term_count >= MAX_TERMINALS) return NULL;
    terminal_t* term = &terminals[term_count++];
    memset(term->buffer, 0, sizeof(term->buffer));
    term->buf_count = 0; term->cmd_len = 0; term->cmd_cursor = 0;
    term->cmd[0] = '\0';
    term->fg_color = RGB(200, 200, 200);
    term->bg_color = RGB(15, 15, 25);
    term->active = true;
    term->win = wm_create_window(x, y, TERM_COLS * 8 + 8, TERM_ROWS * 16 + 8,
                                  "Terminal", term->bg_color);
    if (term->win) term->win->on_draw = term_draw;
    terminal_print_color(term, "WintakOS Terminal v0.9.0", RGB(100, 200, 255));
    terminal_print(term, "'help' yazarak ba\x03lay\x01n.");
    terminal_print(term, "");
    return term;
}
