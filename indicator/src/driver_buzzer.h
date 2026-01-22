#ifndef DRIVER_BUZZER_H
#define DRIVER_BUZZER_H

#include "driver/ledc.h"
#include "esp_log.h"
#include "ascent_r3_hardware_definition.h"

// Initialize the buzzer
esp_err_t buzzer_init(void);

// Play a tone for a specific duration (Hz, ms)
esp_err_t buzz(uint32_t frequency, uint32_t duration_ms);

// Octave base enum (C0 to C8)
typedef enum {
    OCTAVE_0 = 0,
    OCTAVE_1,
    OCTAVE_2,
    OCTAVE_3,
    OCTAVE_4,
    OCTAVE_5,
    OCTAVE_6,
    OCTAVE_7,
    OCTAVE_8
} octave_t;

// Note enum (semitone offset from C)
typedef enum {
    NOTE_C = 0,
    NOTE_CS,
    NOTE_D,
    NOTE_DS,
    NOTE_E,
    NOTE_F,
    NOTE_FS,
    NOTE_G,
    NOTE_GS,
    NOTE_A,
    NOTE_AS,
    NOTE_B
} note_t;

// Play note as square wave
esp_err_t note(note_t note, octave_t octave, uint32_t duration_ms);

esp_err_t note_transition(note_t target_note, octave_t target_octave, uint32_t duration_ms);

esp_err_t note_harmonics(const note_t *notes, const octave_t *octaves, uint8_t count, uint32_t duration_ms);

// Play harmonic waveform from multiple notes
esp_err_t note_harmonics_waveform(const note_t *notes, const octave_t *octaves, uint8_t count, uint32_t duration_ms);

#endif /* DRIVER_BUZZER_H */
