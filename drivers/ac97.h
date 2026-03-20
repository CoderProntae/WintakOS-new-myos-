#ifndef WINTAKOS_AC97_H
#define WINTAKOS_AC97_H

#include "../include/types.h"

bool     ac97_init(void);
void     ac97_play_tone(uint32_t frequency);
void     ac97_stop(void);
bool     ac97_available(void);

#endif
