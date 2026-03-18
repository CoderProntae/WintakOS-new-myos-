#ifndef WINTAKOS_KEYMAP_TR_H
#define WINTAKOS_KEYMAP_TR_H

/*============================================================================
 * Turkce Q Klavye Haritasi - PS/2 Scan Code Set 1
 *
 * VGA text mode CP437 font kullanir. CP437'de bulunan Turkce karakterler:
 *   u-umlaut = '\x81'  (ü)     U-umlaut = '\x9A'  (Ü)
 *   o-umlaut = '\x94'  (ö)     O-umlaut = '\x99'  (Ö)
 *   c-cedilla = '\x87' (ç)     C-cedilla = '\x80'  (Ç)
 *
 * CP437'de BULUNMAYAN Turkce karakterler (Phase 5'te cozulecek):
 *   s-cedilla (ş/Ş)  → gecici: s/S
 *   g-breve   (ğ/Ğ)  → gecici: g/G
 *   dotless-i (ı)     → gecici: i
 *   dotted-I  (İ)     → gecici: I
 *
 * Scancode = fiziksel tus pozisyonu (US layout bazli)
 * Turkce Q fiziksel pozisyon farklari:
 *   US 'i'  (0x17) → TR 'ı' (simdilik 'i')
 *   US ';'  (0x27) → TR 'ş' (simdilik 's')
 *   US '\'' (0x28) → TR 'i'
 *   US '['  (0x1A) → TR 'ğ' (simdilik 'g')
 *   US ']'  (0x1B) → TR 'ü' (CP437: 0x81)
 *   US ','  (0x33) → TR 'ö' (CP437: 0x94)
 *   US '.'  (0x34) → TR 'ç' (CP437: 0x87)
 *   US '/'  (0x35) → TR '.'
 *   US '\'  (0x2B) → TR ','
 *   US '`'  (0x29) → TR '"'
 *   US '-'  (0x0C) → TR '*'
 *   US '='  (0x0D) → TR '-'
 *==========================================================================*/

static const char scancode_to_ascii[128] = {
/*           0     1     2     3     4     5     6     7  */
/* 0x00 */   0,    0,   '1', '2', '3', '4', '5', '6',
/* 0x08 */  '7', '8', '9', '0', '*', '-',  0,    0,
/* 0x10 */  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',
/* 0x18 */  'o', 'p', 'g','\x81', 0,   0,  'a', 's',
/* 0x20 */  'd', 'f', 'g', 'h', 'j', 'k', 'l', 's',
/* 0x28 */  'i', '"',  0,  ',', 'z', 'x', 'c', 'v',
/* 0x30 */  'b', 'n', 'm','\x94','\x87','.',0, '*',
/* 0x38 */   0,  ' ',  0,    0,    0,    0,    0,    0,
/* 0x40 */   0,    0,    0,    0,    0,    0,    0,  '7',
/* 0x48 */  '8', '9', '-', '4', '5', '6', '+', '1',
/* 0x50 */  '2', '3', '0', '.',  0,    0,  '<',  0,
/* 0x58 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x68 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x78 */   0,    0,    0,    0,    0,    0,    0,    0,
};

static const char scancode_to_ascii_shift[128] = {
/*           0     1     2     3     4     5     6     7  */
/* 0x00 */   0,    0,   '!','\'', '^', '+', '%', '&',
/* 0x08 */  '/', '(', ')', '=', '?', '_',  0,    0,
/* 0x10 */  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',
/* 0x18 */  'O', 'P', 'G','\x9A', 0,   0,  'A', 'S',
/* 0x20 */  'D', 'F', 'G', 'H', 'J', 'K', 'L', 'S',
/* 0x28 */  'I',  0,   0,  ';', 'Z', 'X', 'C', 'V',
/* 0x30 */  'B', 'N', 'M','\x99','\x80',':',0, '*',
/* 0x38 */   0,  ' ',  0,    0,    0,    0,    0,    0,
/* 0x40 */   0,    0,    0,    0,    0,    0,    0,  '7',
/* 0x48 */  '8', '9', '-', '4', '5', '6', '+', '1',
/* 0x50 */  '2', '3', '0', '.',  0,    0,  '>',  0,
/* 0x58 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x60 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x68 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x70 */   0,    0,    0,    0,    0,    0,    0,    0,
/* 0x78 */   0,    0,    0,    0,    0,    0,    0,    0,
};

#endif
