#include "speaker.h"
#include "../cpu/ports.h"
#include "../cpu/pit.h"

#define PIT_BASE_FREQ  1193182
#define PIT_CHANNEL2   0x42
#define PIT_CMD        0x43
#define SPEAKER_PORT   0x61

void speaker_init(void)
{
    speaker_stop();
}

void speaker_play_tone(uint32_t frequency)
{
    if (frequency == 0) {
        speaker_stop();
        return;
    }

    uint32_t divisor = PIT_BASE_FREQ / frequency;
    if (divisor > 65535) divisor = 65535;
    if (divisor < 1) divisor = 1;

    /* PIT Channel 2: square wave mode */
    outb(PIT_CMD, 0xB6);
    outb(PIT_CHANNEL2, (uint8_t)(divisor & 0xFF));
    outb(PIT_CHANNEL2, (uint8_t)((divisor >> 8) & 0xFF));

    /* Speaker gate acik + speaker enable */
    uint8_t val = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, val | 0x03);
}

void speaker_stop(void)
{
    uint8_t val = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, val & ~0x03);
}

/* Blocking bekleme (ms cinsinden) */
static void delay_ms(uint32_t ms)
{
    uint32_t ticks = (ms * PIT_FREQUENCY) / 1000;
    if (ticks == 0) ticks = 1;
    uint32_t start = pit_get_ticks();
    while (pit_get_ticks() - start < ticks) {
        __asm__ volatile("hlt");
    }
}

void speaker_beep(uint32_t frequency, uint32_t duration_ms)
{
    speaker_play_tone(frequency);
    delay_ms(duration_ms);
    speaker_stop();
}

void speaker_play_melody(const note_t* notes, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        if (notes[i].frequency == NOTE_NONE) {
            speaker_stop();
            delay_ms(notes[i].duration_ms);
        } else {
            speaker_beep(notes[i].frequency, notes[i].duration_ms);
        }
        delay_ms(20); /* Notalar arasi kisa bosluk */
    }
}

void sound_startup(void)
{
    static const note_t melody[] = {
        {NOTE_C5,  100},
        {NOTE_E5,  100},
        {NOTE_G5,  100},
        {NOTE_C6,  200},
    };
    speaker_play_melody(melody, 4);
}

void sound_error(void)
{
    static const note_t melody[] = {
        {NOTE_A4, 150},
        {NOTE_NONE, 50},
        {NOTE_A4, 150},
    };
    speaker_play_melody(melody, 3);
}

void sound_click(void)
{
    speaker_beep(800, 10);
}

void sound_notify(void)
{
    static const note_t melody[] = {
        {NOTE_E5, 80},
        {NOTE_G5, 120},
    };
    speaker_play_melody(melody, 2);
}

void sound_close(void)
{
    static const note_t melody[] = {
        {NOTE_G5, 60},
        {NOTE_E5, 60},
        {NOTE_C5, 80},
    };
    speaker_play_melody(melody, 3);
}
