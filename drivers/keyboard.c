#include "keyboard.h"
#include "../cpu/ports.h"
#include "../cpu/pic.h"
#include "../cpu/isr.h"
#include "../kernel/vga.h"

static volatile char buffer[256];
static volatile uint32_t head = 0, tail = 0;
static bool shift_pressed = false;
static bool caps_lock = false;

// ===================== TÜRKÇE Q KLAVYE TABLOSU (GÜNCEL) =====================
static const char tr_q_map[128][2] = {
    {0,0}, {27,27}, {'1','!'}, {'2','"'}, {'3','^'}, {'4','+'}, {'5','%'}, 
    {'6','&'}, {'7','/'}, {'8','('}, {'9',')'}, {'0','='}, {'*','?'}, {'-','_'}, 
    {'\b','\b'}, {'\t','\t'},
    
    {'q','Q'}, {'w','W'}, {'e','E'}, {'r','R'}, {'t','T'}, {'y','Y'}, {'u','U'}, 
    {'ı','I'}, {'o','O'}, {'p','P'}, {'ğ','Ğ'}, {'ü','Ü'}, {'\n','\n'}, 
    {0,0}, {'a','A'},
    
    {'s','S'}, {'d','D'}, {'f','F'}, {'g','G'}, {'h','H'}, {'j','J'}, {'k','K'}, 
    {'l','L'}, {'ş','Ş'}, {'i','İ'}, {',',';'}, {'.',':'}, {'<','>'}, {0,0}, 
    {'z','Z'},
    
    {'x','X'}, {'c','C'}, {'v','V'}, {'b','B'}, {'n','N'}, {'m','M'}, {'ö','Ö'}, 
    {'ç','Ç'}, {'.','>'}, {'-','_'}
};

static void keyboard_handler(registers_t* r) {
    (void)r;
    uint8_t sc = inb(0x60);

    if (sc == 0xE0) { inb(0x60); pic_send_eoi(1); return; }   // Ok tuşları

    if (sc == 0x2A || sc == 0x36)      shift_pressed = true;
    else if (sc == 0xAA || sc == 0xB6) shift_pressed = false;
    else if (sc == 0x3A)                caps_lock = !caps_lock;
    else if (sc < 128 && tr_q_map[sc][0]) {
        bool upper = shift_pressed ^ caps_lock;
        char ch = tr_q_map[sc][upper ? 1 : 0];
        buffer[head] = ch;
        head = (head + 1) % 256;
    }

    pic_send_eoi(1);
}

void keyboard_init(void) {
    head = tail = 0;
    shift_pressed = false;
    caps_lock = false;
    
    irq_register_handler(1, keyboard_handler);
    pic_clear_mask(1);
}

char keyboard_getchar(void) {
    while (head == tail) __asm__ volatile("hlt");
    char c = buffer[tail];
    tail = (tail + 1) % 256;
    return c;
}

void keyboard_clear_buffer(void) {
    head = tail = 0;
}
