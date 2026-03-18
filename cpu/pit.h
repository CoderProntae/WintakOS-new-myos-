#ifndef WINTAKOS_PIT_H
#define WINTAKOS_PIT_H

#include "../include/types.h"

#define PIT_FREQUENCY  100

void     pit_init(uint32_t frequency);
uint32_t pit_get_ticks(void);

#endif
