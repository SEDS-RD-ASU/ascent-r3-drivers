#include "driver_buzzer.h"
#include "esp_rom_sys.h"
#include <math.h>

#define BUZZER_LEDC_TIMER          LEDC_TIMER_0
#define BUZZER_LEDC_MODE           LEDC_LOW_SPEED_MODE
#define BUZZER_LEDC_CHANNEL        LEDC_CHANNEL_0
#define BUZZER_LEDC_DUTY_RES       LEDC_TIMER_13_BIT
#define BUZZER_LEDC_MAX_DUTY       ((1 << 13) - 1)
#define BUZZER_LEDC_FREQ_MIN       20
#define BUZZER_LEDC_FREQ_MAX       50000
#define BUZZER_DEFAULT_FREQ        440 // A4

#define SINE_TABLE_SIZE 64
static uint16_t sine_table[SINE_TABLE_SIZE];

static note_t current_note = NOTE_C;
static octave_t current_octave = OCTAVE_4;

static const char *TAG = "BUZZER DRIVER";

// ---------- Initialization ----------
esp_err_t buzzer_init(void)
{
    // Generate sine lookup table
    for (int i = 0; i < SINE_TABLE_SIZE; i++) {
        float angle = (2.0f * M_PI * i) / SINE_TABLE_SIZE;
        float sine_val = (sinf(angle) + 1.0f) / 2.0f;
        sine_table[i] = (uint16_t)(sine_val * BUZZER_LEDC_MAX_DUTY);
    }

    // Configure LEDC timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode = BUZZER_LEDC_MODE,
        .timer_num  = BUZZER_LEDC_TIMER,
        .duty_resolution = BUZZER_LEDC_DUTY_RES,
        .freq_hz = BUZZER_DEFAULT_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Configure LEDC channel
    ledc_channel_config_t ledc_channel = {
        .speed_mode = BUZZER_LEDC_MODE,
        .channel    = BUZZER_LEDC_CHANNEL,
        .timer_sel  = BUZZER_LEDC_TIMER,
        .intr_type  = LEDC_INTR_DISABLE,
        #ifdef R2
        .gpio_num   = R2_PIN_BUZZER,
        #endif
        #ifndef R2
        .gpio_num = PIN_BUZZER,
        #endif
        .duty       = 0,
        .hpoint     = 0
    };

    #ifdef R2
    ESP_LOGW(TAG, "BUZZER DRIVER CONFIGURED FOR R2! REMOVE #define R2 IF THIS IS NOT A R2 BOARD");
    #endif
    ESP_LOGI(TAG, "SUCCESSFULLY INITIALIZED BUZZER");

    return ledc_channel_config(&ledc_channel);
}

// ---------- Tone Control ----------
static esp_err_t tone(uint32_t frequency)
{
    if (frequency < BUZZER_LEDC_FREQ_MIN || frequency > BUZZER_LEDC_FREQ_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    ESP_ERROR_CHECK(ledc_set_freq(BUZZER_LEDC_MODE, BUZZER_LEDC_TIMER, frequency));
    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, BUZZER_LEDC_MAX_DUTY / 2));
    return ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

static esp_err_t noTone(void)
{
    ESP_ERROR_CHECK(ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, 0));
    return ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
}

esp_err_t buzz(uint32_t frequency, uint32_t duration_ms)
{
    esp_err_t result = tone(frequency);
    if (result != ESP_OK) {
        return result;
    }

    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    return noTone();
}

// ---------- Frequency Calculation ----------
static double calculate_frequency(note_t note, octave_t octave)
{
    int note_index = (octave * 12) + note;
    int semitone_offset = note_index - 57; // A4 = 440Hz at index 57
    return 440.0 * pow(2.0, semitone_offset / 12.0);
}

// ---------- Note Playback - Square ----------
esp_err_t note(note_t note, octave_t octave, uint32_t duration_ms)
{
    double freq = calculate_frequency(note, octave);
    if (freq < BUZZER_LEDC_FREQ_MIN || freq > BUZZER_LEDC_FREQ_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    #ifndef DISABLE_BUZZER
    return buzz((uint32_t)(freq + 0.5), duration_ms);
    #else
    return ESP_OK;
    #endif
}

esp_err_t note_transition(note_t target_note, octave_t target_octave, uint32_t duration_ms)
{
    double start_freq = calculate_frequency(current_note, current_octave);
    double end_freq = calculate_frequency(target_note, target_octave);

    if (start_freq < BUZZER_LEDC_FREQ_MIN || end_freq > BUZZER_LEDC_FREQ_MAX) {
        return ESP_ERR_INVALID_ARG;
    }

    // Dynamic steps: Aim for ~5ms per step
    uint32_t ideal_step_duration_ms = 5;
    uint32_t steps = duration_ms / ideal_step_duration_ms;
    if (steps < 1) steps = 1;
    if (steps > 50) steps = 50;

    uint32_t step_delay_us = (duration_ms * 1000UL) / steps;

    for (uint32_t i = 0; i <= steps; i++) {
        float t = (float)i / steps;  // 0.0 to 1.0
        double freq = start_freq * pow(end_freq / start_freq, t);

        tone((uint32_t)(freq + 0.5));
        esp_rom_delay_us(step_delay_us);  // Precise delay for all durations
    }

    noTone();
    current_note = target_note;
    current_octave = target_octave;
    return ESP_OK;
}

esp_err_t note_harmonics(const note_t *notes, const octave_t *octaves, uint8_t count, uint32_t duration_ms)
{
    if (count < 2 || count > 5 || notes == NULL || octaves == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    double freqs[5];
    for (uint8_t i = 0; i < count; i++) {
        freqs[i] = calculate_frequency(notes[i], octaves[i]);
        if (freqs[i] < BUZZER_LEDC_FREQ_MIN || freqs[i] > BUZZER_LEDC_FREQ_MAX) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    const uint32_t cycle_us = 2000;  // 2ms per cycle (~500Hz switching)
    const uint32_t slice_us = cycle_us / count;

    uint32_t total_cycles = (duration_ms * 1000UL) / cycle_us;

    for (uint32_t c = 0; c < total_cycles; c++) {
        for (uint8_t i = 0; i < count; i++) {
            tone((uint32_t)(freqs[i] + 0.5));
            esp_rom_delay_us(slice_us);
        }
    }

    noTone();
    return ESP_OK;
}

esp_err_t note_harmonics_waveform(const note_t *notes, const octave_t *octaves, uint8_t count, uint32_t duration_ms)
{
    if (count < 2 || count > 5 || notes == NULL || octaves == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    double freqs[5];
    for (uint8_t i = 0; i < count; i++) {
        freqs[i] = calculate_frequency(notes[i], octaves[i]);
        if (freqs[i] < BUZZER_LEDC_FREQ_MIN || freqs[i] > BUZZER_LEDC_FREQ_MAX) {
            return ESP_ERR_INVALID_ARG;
        }
    }

    // ---------- Waveform Generation ----------
    const int WAVEFORM_SIZE = 256;
    uint16_t waveform[WAVEFORM_SIZE];
    double max_val = 0.0;

    // Step 1: Use lowest frequency as base
    double base_freq = freqs[0];
    for (uint8_t i = 1; i < count; i++) {
        if (freqs[i] < base_freq) base_freq = freqs[i];
    }

    // Step 2: Generate phase-aligned, weighted harmonic waveform
    for (int i = 0; i < WAVEFORM_SIZE; i++) {
        double t = (double)i / WAVEFORM_SIZE;  // Normalized cycle [0.0, 1.0)
        double sample = 0.0;

        for (uint8_t j = 0; j < count; j++) {
            double multiple = freqs[j] / base_freq;
            double weight = 1.0 / multiple;  // Weight higher frequencies less
            sample += weight * sin(2.0 * M_PI * multiple * t);  // Phase aligned
        }

        if (fabs(sample) > max_val) max_val = fabs(sample);
        waveform[i] = sample;  // Temporarily store raw
    }

    // Step 3: Normalize to 13-bit PWM duty range
    for (int i = 0; i < WAVEFORM_SIZE; i++) {
        double normalized = (waveform[i] / max_val + 1.0) / 2.0;  // 0.0 to 1.0
        waveform[i] = (uint16_t)(normalized * BUZZER_LEDC_MAX_DUTY);
    }

    // ---------- Playback ----------
    uint32_t sample_rate = base_freq * WAVEFORM_SIZE;
    uint32_t delay_us = 1000000UL / sample_rate;
    uint32_t total_samples = (duration_ms * 1000UL) / delay_us;

    for (uint32_t i = 0; i < total_samples; i++) {
        uint16_t duty = waveform[i % WAVEFORM_SIZE];
        ledc_set_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL, duty);
        ledc_update_duty(BUZZER_LEDC_MODE, BUZZER_LEDC_CHANNEL);
        esp_rom_delay_us(delay_us);
    }

    noTone();
    return ESP_OK;
}
