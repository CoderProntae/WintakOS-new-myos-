#ifndef WINTAKOS_KEYMAP_TR_H
#define WINTAKOS_KEYMAP_TR_H

/*============================================================================
 * Türkçe Q Klavye Düzeni - Scancode Set 1
 *
 * PS/2 klavye scancode'ları (Set 1) → ASCII/karakter dönüşümü.
 * İndeks = scancode numarası (0x00 - 0x3A).
 * 0 = tanımsız/özel tuş (ayrıca işlenir).
 *
 * NOT: Türkçe karakterler (ç, ş, ğ, ü, ö, ı, İ) VGA text mode'da
 * doğrudan gösterilemez (Code Page 437 sınırlaması).
 * Phase 5'te (VESA + Font motoru) tam UTF-8 desteği eklenecek.
 * Şu an ASCII yakınları kullanılıyor, VESA'da gerçek Türkçe gelecek.
 *==========================================================================*/

/* Normal (shift yok) */
static const char scancode_to_char[128] = {
    0,                          /* 0x00: hata */
    0,                          /* 0x01: Escape */
    '1','2','3','4','5','6',    /* 0x02 - 0x07 */
    '7','8','9','0',            /* 0x08 - 0x0B */
    '*','-',                    /* 0x0C - 0x0D: * ve - (TR Q: * -) */
    0,                          /* 0x0E: Backspace */
    0,                          /* 0x0F: Tab */
    'q','w','e','r','t','y',    /* 0x10 - 0x15 */
    'u','i','o','p',            /* 0x16 - 0x19: u ı(i) o p */
    '[',']',                    /* 0x1A - 0x1B: gğ üü (ASCII fallback) */
    0,                          /* 0x1C: Enter */
    0,                          /* 0x1D: Left Ctrl */
    'a','s','d','f','g','h',    /* 0x1E - 0x23 */
    'j','k','l',                /* 0x24 - 0x26 */
    ';','\'',                   /* 0x27 - 0x28: şş iİ (ASCII fallback) */
    '`',                        /* 0x29: " (backtick) */
    0,                          /* 0x2A: Left Shift */
    '\\',                       /* 0x2B: , (backslash) */
    'z','x','c','v','b','n',    /* 0x2C - 0x31 */
    'm',                        /* 0x32 */
    ',','.',                    /* 0x33 - 0x34: öö çç (ASCII fallback) */
    '/',                        /* 0x35 */
    0,                          /* 0x36: Right Shift */
    '*',                        /* 0x37: Keypad * */
    0,                          /* 0x38: Left Alt */
    ' ',                        /* 0x39: Space */
    0                           /* 0x3A: CapsLock */
};

/* Shift basılı */
static const char scancode_to_char_shift[128] = {
    0,
    0,                          /* Escape */
    '!','"','#','$','%','&',    /* Shift + 1-6 (TR Q) */
    '/','(',')','=',            /* Shift + 7-0 (TR Q) */
    '?','_',                    /* Shift + * - */
    0,                          /* Backspace */
    0,                          /* Tab */
    'Q','W','E','R','T','Y',
    'U','I','O','P',            /* Shift: U İ(I) O P */
    '{','}',                    /* Shift + [] */
    0,                          /* Enter */
    0,                          /* Ctrl */
    'A','S','D','F','G','H',
    'J','K','L',
    ':','"',                    /* Shift + ;' */
    '~',                        /* Shift + ` */
    0,                          /* Left Shift */
    '|',                        /* Shift + \ */
    'Z','X','C','V','B','N',
    'M',
    '<','>',                    /* Shift + , . */
    '?',                        /* Shift + / */
    0,                          /* Right Shift */
    '*',
    0,                          /* Alt */
    ' ',
    0                           /* CapsLock */
};

#endif
