#ifndef WINTAKOS_SPEAKER_H
#define WINTAKOS_SPEAKER_H

#include "../include/types.h"

#define NOTE_NONE  0
#define NOTE_C4    262
#define NOTE_CS4   277
#define NOTE_D4    294
#define NOTE_DS4   311
#define NOTE_E4    330
#define NOTE_F4    349
#define NOTE_FS4   370
#define NOTE_G4    392
#define NOTE_GS4   415
#define NOTE_A4    440
#define NOTE_AS4   466
#define NOTE_B4    494
#define NOTE_C5    523
#define NOTE_CS5   554
#define NOTE_D5    587
#define NOTE_DS5   622
#define NOTE_E5    659
#define NOTE_F5    698
#define NOTE_FS5   740
#define NOTE_G5    784
#define NOTE_GS5   831
#define NOTE_A5    880
#define NOTE_AS5   932
#define NOTE_B5    988
#define NOTE_C6    1047

typedef struct {
    uint16_t frequency;
    uint16_t duration_ms;
} note_t;

void speaker_init(void);
void speaker_play_tone(uint32_t frequency);
void speaker_stop(void);
void speaker_beep(uint32_t frequency, uint32_t duration_ms);
void speaker_play_melody(const note_t* notes, uint32_t count);

void sound_startup(void);
void sound_error(void);
void sound_click(void);
void sound_notify(void);
void sound_close(void);

/* Ses sistemi bilgisi */
const char* sound_get_driver_name(void);

#endif
