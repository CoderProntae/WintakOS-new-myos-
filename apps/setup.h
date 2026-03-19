#ifndef WINTAKOS_SETUP_H
#define WINTAKOS_SETUP_H

#include "../include/types.h"

#define SETUP_MAX_USERNAME  20

typedef struct {
    char     username[SETUP_MAX_USERNAME + 1];
    uint8_t  theme;          /* 0=koyu, 1=acik, 2=mavi */
    bool     completed;
} setup_config_t;

/* Kurulum sihirbazini baslat — true donerse kurulum tamamlandi */
bool setup_run(void);

/* Kayitli ayarlari oku */
setup_config_t* setup_get_config(void);

#endif
