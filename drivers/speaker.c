#include "speaker.h"
#include "ac97.h"
#include "../cpu/ports.h"
#include "../cpu/pit.h"

#define PIT_BASE_FREQ  1193182
#define PIT_CHANNEL2   0x42
#define PIT_CMD        0x43
#define SPEAKER_PORT   0x61

static bool use_ac97 = false;

static void pcspk_stop(void)
{
    uint8_t val = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, val & ~0x03);
}

static void pcspk_play(uint32_t frequency)
{
    if (frequency == 0) { pcspk_stop(); return; }
    uint32_t div = PIT_BASE_FREQ / frequency;
    if (div > 65535) div = 65535;
    if (div < 1) div = 1;
    outb(PIT_CMD, 0xB6);
    outb(PIT_CHANNEL2, (uint8_t)(div & 0xFF));
    outb(PIT_CHANNEL2, (uint8_t)((div >> 8) & 0xFF));
    uint8_t val = inb(SPEAKER_PORT);
    outb(SPEAKER_PORT, val | 0x03);
}

static void delay_ms(uint32_t ms)
{
    uint32_t ticks = (ms * PIT_FREQUENCY) / 1000;
    if (ticks == 0) ticks = 1;
    uint32_t start = pit_get_ticks();
    while (pit_get_ticks() - start < ticks)
        __asm__ volatile("hlt");
}

void speaker_init(void)
{
    pcspk_stop();
    if (ac97_init()) use_ac97 = true;
    else use_ac97 = false;
}

void speaker_play_tone(uint32_t frequency)
{
    if (use_ac97) ac97_play_tone(frequency);
    pcspk_play(frequency);
}

void speaker_stop(void)
{
    if (use_ac97) ac97_stop();
    pcspk_stop();
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
        delay_ms(20);
    }
}

void sound_startup(void)
{
    static const note_t m[] = {
        {NOTE_C5, 100}, {NOTE_E5, 100}, {NOTE_G5, 100}, {NOTE_C6, 200}
    };
    speaker_play_melody(m, 4);
}

void sound_error(void)
{
    static const note_t m[] = {
        {NOTE_A4, 150}, {NOTE_NONE, 50}, {NOTE_A4, 150}
    };
    speaker_play_melody(m, 3);
}

void sound_click(void)
{
    speaker_beep(800, 10);
}

void sound_notify(void)
{
    static const note_t m[] = { {NOTE_E5, 80}, {NOTE_G5, 120} };
    speaker_play_melody(m, 2);
}

void sound_close(void)
{
    static const note_t m[] = {
        {NOTE_G5, 60}, {NOTE_E5, 60}, {NOTE_C5, 80}
    };
    speaker_play_melody(m, 3);
}

const char* sound_get_driver_name(void)
{
    if (use_ac97) return "AC'97 + PC Speaker";
    return "PC Speaker";
}
