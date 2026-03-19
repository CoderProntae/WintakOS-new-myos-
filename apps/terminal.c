#include "terminal.h"
#include "../gui/window.h"
#include "../gui/widget.h"
#include "../drivers/framebuffer.h"
#include "../drivers/font8x16.h"
#include "../cpu/pit.h"
#include "../cpu/rtc.h"
#include "../memory/pmm.h"
#include "../memory/heap.h"
#include "../lib/string.h"

/* Forward decl */
static void term_draw(window_t* win);
static void term_execute(terminal_t* term);

/* Aktif terminal listesi */
#define MAX_TERMINALS 4
static terminal_t terminals[MAX_TERMINALS];
static uint32_t term_count = 0;

static void draw_char_at(uint32_t px, uint32_t py, uint8_t c, uint32_t fg, uint32_t bg)
{
    const uint8_t* glyph = font8x16_data[c < 128 ? c : 0];
    for (uint32_t y = 0; y < 16; y++) {
        uint8_t line = glyph[y];
        for (uint32_t x = 0; x < 8; x++) {
            fb_put_pixel(px + x, py + y, (line & (0x80 >> x)) ? fg : bg);
        }
    }
}

/* Terminal icin pencere on_draw callback */
static void term_draw(window_t* win)
{
    /* Hangi terminal bu pencereye ait? */
    terminal_t* term = NULL;
    for (uint32_t i = 0; i < term_count; i++) {
        if (terminals[i].win == win) {
            term = &terminals[i];
            break;
        }
    }
    if (!term) return;

    uint32_t px0 = (uint32_t)win->x + 4;
    uint32_t py0 = (uint32_t)win->y + 4;

    /* Satir tamponu ciz */
    uint32_t visible = TERM_ROWS - 1; /* son satir komut icin */
    uint32_t start = 0;
    if (term->buf_count > visible) {
        start = term->buf_count - visible;
    }

    for (uint32_t i = 0; i < visible && (start + i) < term->buf_count; i++) {
        const char* line = term->buffer[(start + i) % TERM_BUF_LINES];
        uint32_t px = px0;
        uint32_t py = py0 + i * 16;

        for (uint32_t j = 0; line[j] && j < TERM_COLS; j++) {
            draw_char_at(px, py, (uint8_t)line[j], term->fg_color, term->bg_color);
            px += 8;
        }
    }

    /* Komut satiri */
    uint32_t cmd_y = py0 + visible * 16;

    /* Prompt */
    draw_char_at(px0, cmd_y, '>', RGB(100, 255, 100), term->bg_color);
    draw_char_at(px0 + 8, cmd_y, ' ', term->fg_color, term->bg_color);

    /* Komut metni */
    uint32_t cpx = px0 + 16;
    for (uint32_t i = 0; i < term->cmd_len && i < TERM_COLS - 3; i++) {
        draw_char_at(cpx, cmd_y, (uint8_t)term->cmd[i], term->fg_color, term->bg_color);
        cpx += 8;
    }

    /* Cursor (yanip sonen blok) */
    uint32_t blink = (pit_get_ticks() / 25) % 2;
    if (blink && term->active) {
        draw_char_at(cpx, cmd_y, '_', RGB(100, 255, 100), term->bg_color);
    } else {
        draw_char_at(cpx, cmd_y, ' ', term->fg_color, term->bg_color);
    }
}

/* Tampona satir ekle */
static void term_add_line(terminal_t* term, const char* line)
{
    uint32_t idx = term->buf_count % TERM_BUF_LINES;
    uint32_t i;
    for (i = 0; i < TERM_COLS && line[i]; i++) {
        term->buffer[idx][i] = line[i];
    }
    term->buffer[idx][i] = '\0';
    term->buf_count++;
}

/* String'i satir satir ekle (\n'de bol) */
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
            line[pos++] = *str;
            str++;
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

/* Basit strcmp */
static bool str_eq(const char* a, const char* b)
{
    while (*a && *b) {
        if (*a != *b) return false;
        a++; b++;
    }
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

/* Komut calistir */
static void term_execute(terminal_t* term)
{
    /* Komutu tampona yaz */
    char prompt_line[TERM_COLS + 1];
    prompt_line[0] = '>';
    prompt_line[1] = ' ';
    uint32_t i;
    for (i = 0; i < term->cmd_len && i < TERM_COLS - 2; i++) {
        prompt_line[i + 2] = term->cmd[i];
    }
    prompt_line[i + 2] = '\0';
    term_add_line(term, prompt_line);

    if (term->cmd_len == 0) {
        /* Bos komut */
    }
    else if (str_eq(term->cmd, "help")) {
        terminal_print_color(term, "Kullanilabilir komutlar:", RGB(100, 200, 255));
        terminal_print(term, "  help      - Bu yardim mesaji");
        terminal_print(term, "  clear     - Ekrani temizle");
        terminal_print(term, "  sysinfo   - Sistem bilgileri");
        terminal_print(term, "  uptime    - Calisma suresi");
        terminal_print(term, "  time      - Saat ve tarih");
        terminal_print(term, "  mem       - Bellek durumu");
        terminal_print(term, "  echo <..> - Metin yazdir");
        terminal_print(term, "  ver       - Surum bilgisi");
    }
    else if (str_eq(term->cmd, "clear")) {
        term->buf_count = 0;
        memset(term->buffer, 0, sizeof(term->buffer));
    }
    else if (str_eq(term->cmd, "sysinfo")) {
        terminal_print_color(term, "=== Sistem Bilgisi ===", RGB(255, 200, 100));
        terminal_print(term, "OS:      WintakOS v0.7.0");
        terminal_print(term, "CPU:     i386 Protected Mode");
        terminal_print(term, "RAM:     128 MB");
        terminal_print(term, "Video:   800x600x32 VESA");
        terminal_print(term, "Klavye:  PS/2 Turkce Q");
        terminal_print(term, "Mouse:   PS/2");
    }
    else if (str_eq(term->cmd, "uptime")) {
        uint32_t sec = pit_get_ticks() / PIT_FREQUENCY;
        uint32_t h = sec / 3600;
        uint32_t m = (sec % 3600) / 60;
        uint32_t s = sec % 60;

        char buf[32];
        buf[0] = '0' + (char)(h / 10); buf[1] = '0' + (char)(h % 10);
        buf[2] = ':';
        buf[3] = '0' + (char)(m / 10); buf[4] = '0' + (char)(m % 10);
        buf[5] = ':';
        buf[6] = '0' + (char)(s / 10); buf[7] = '0' + (char)(s % 10);
        buf[8] = '\0';
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "time")) {
        rtc_time_t t;
        rtc_read(&t);
        char buf[32];
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
        terminal_print_color(term, "=== Bellek Durumu ===", RGB(255, 200, 100));

        char buf[48];
        uint32_t free_p = pmm_get_free_pages();
        uint32_t total_p = pmm_get_total_pages();
        uint32_t used_p = pmm_get_used_pages();

        /* Basit formatlama */
        terminal_print(term, "PMM:");

        /* Total */
        uint32_t pos = 0;
        const char* l1 = "  Toplam: ";
        while (*l1) buf[pos++] = *l1++;
        pos += 0; /* sayi eklenecek */
        buf[pos] = '\0';
        terminal_print(term, buf);

        /* Sayi yazdirma icin tekrar */
        char nbuf[16];
        uint32_t npos;

        /* Total pages */
        npos = 0;
        if (total_p == 0) { nbuf[npos++] = '0'; }
        else {
            char tmp[12]; uint32_t tp = 0;
            uint32_t v = total_p;
            while (v > 0) { tmp[tp++] = '0' + (v % 10); v /= 10; }
            while (tp > 0) nbuf[npos++] = tmp[--tp];
        }
        nbuf[npos] = '\0';

        pos = 0;
        const char* prefix = "  Toplam: ";
        while (*prefix) buf[pos++] = *prefix++;
        for (uint32_t j = 0; j < npos; j++) buf[pos++] = nbuf[j];
        const char* suffix = " sayfa";
        while (*suffix) buf[pos++] = *suffix++;
        buf[pos] = '\0';
        /* Onceki "Toplam" satirini sil, dogru olani yaz */
        term->buf_count--; /* son satiri geri al */
        terminal_print(term, buf);

        /* Free */
        npos = 0;
        if (free_p == 0) { nbuf[npos++] = '0'; }
        else {
            char tmp[12]; uint32_t tp = 0;
            uint32_t v = free_p;
            while (v > 0) { tmp[tp++] = '0' + (v % 10); v /= 10; }
            while (tp > 0) nbuf[npos++] = tmp[--tp];
        }
        nbuf[npos] = '\0';
        pos = 0;
        prefix = "  Bos:    ";
        while (*prefix) buf[pos++] = *prefix++;
        for (uint32_t j = 0; j < npos; j++) buf[pos++] = nbuf[j];
        suffix = " sayfa";
        while (*suffix) buf[pos++] = *suffix++;
        buf[pos] = '\0';
        terminal_print(term, buf);

        /* Used */
        npos = 0;
        if (used_p == 0) { nbuf[npos++] = '0'; }
        else {
            char tmp[12]; uint32_t tp = 0;
            uint32_t v = used_p;
            while (v > 0) { tmp[tp++] = '0' + (v % 10); v /= 10; }
            while (tp > 0) nbuf[npos++] = tmp[--tp];
        }
        nbuf[npos] = '\0';
        pos = 0;
        prefix = "  Kullan: ";
        while (*prefix) buf[pos++] = *prefix++;
        for (uint32_t j = 0; j < npos; j++) buf[pos++] = nbuf[j];
        suffix = " sayfa";
        while (*suffix) buf[pos++] = *suffix++;
        buf[pos] = '\0';
        terminal_print(term, buf);

        terminal_print(term, "Heap:");
        /* heap free */
        npos = 0;
        {
            uint32_t v = heap_get_free();
            if (v == 0) { nbuf[npos++] = '0'; }
            else {
                char tmp[12]; uint32_t tp = 0;
                while (v > 0) { tmp[tp++] = '0' + (v % 10); v /= 10; }
                while (tp > 0) nbuf[npos++] = tmp[--tp];
            }
        }
        nbuf[npos] = '\0';
        pos = 0;
        prefix = "  Bos:    ";
        while (*prefix) buf[pos++] = *prefix++;
        for (uint32_t j = 0; j < npos; j++) buf[pos++] = nbuf[j];
        suffix = " byte";
        while (*suffix) buf[pos++] = *suffix++;
        buf[pos] = '\0';
        terminal_print(term, buf);
    }
    else if (str_eq(term->cmd, "ver")) {
        terminal_print_color(term, "WintakOS v0.7.0", RGB(100, 200, 255));
        terminal_print(term, "Milestone 6 - Terminal/Shell");
    }
    else if (str_starts(term->cmd, "echo ")) {
        terminal_print(term, term->cmd + 5);
    }
    else {
        terminal_print_color(term, "Bilinmeyen komut. 'help' yazin.", RGB(255, 100, 100));
    }

    term->cmd_len = 0;
    term->cmd[0] = '\0';
    wm_set_dirty();
}

void terminal_input_char(terminal_t* term, uint8_t c)
{
    if (!term || !term->active) return;

    if (c == '\n') {
        term_execute(term);
    }
    else if (c == '\b') {
        if (term->cmd_len > 0) {
            term->cmd_len--;
            term->cmd[term->cmd_len] = '\0';
            wm_set_dirty();
        }
    }
    else if (c >= 0x20 || c <= 0x10) {
        /* Yazdırılabilir karakter veya Türkçe özel (0x01-0x10) */
        if (term->cmd_len < TERM_CMD_MAX - 1) {
            term->cmd[term->cmd_len] = (char)c;
            term->cmd_len++;
            term->cmd[term->cmd_len] = '\0';
            wm_set_dirty();
        }
    }
}

terminal_t* terminal_create(int32_t x, int32_t y)
{
    if (term_count >= MAX_TERMINALS) return NULL;

    terminal_t* term = &terminals[term_count];
    term_count++;

    memset(term->buffer, 0, sizeof(term->buffer));
    term->buf_count = 0;
    term->buf_top = 0;
    term->cmd_len = 0;
    term->cmd[0] = '\0';
    term->fg_color = RGB(200, 200, 200);
    term->bg_color = RGB(15, 15, 25);
    term->active = true;

    term->win = wm_create_window(x, y, TERM_COLS * 8 + 8, TERM_ROWS * 16 + 8,
                                  "Terminal", term->bg_color);

    if (term->win) {
        term->win->on_draw = term_draw;
    }

    terminal_print_color(term, "WintakOS Terminal v0.7.0", RGB(100, 200, 255));
    terminal_print(term, "'help' yazarak baslayın.");
    terminal_print(term, "");

    return term;
}
