#ifndef WINTAKOS_MOUSE_H
#define WINTAKOS_MOUSE_H

#include "../include/types.h"

typedef struct {
    int32_t  x;
    int32_t  y;
    uint8_t  buttons;    /* bit0=sol, bit1=sag, bit2=orta */
    bool     moved;
} mouse_state_t;

#define MOUSE_BTN_LEFT   0x01
#define MOUSE_BTN_RIGHT  0x02
#define MOUSE_BTN_MIDDLE 0x04

void mouse_init(uint32_t screen_w, uint32_t screen_h);
mouse_state_t mouse_get_state(void);
void mouse_clear_moved(void);

#endif
