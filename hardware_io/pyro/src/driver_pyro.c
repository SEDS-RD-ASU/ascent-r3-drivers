#include "driver_pyro.h"
#include "ascent_r3_hardware_definition.h"
#include "adc_oneshot.h"
#include "esp_log.h"
#include "adc_cali.h"
#include "adc_cali_scheme.h"
#include "driver_psu.h"
#include "esp_timer.h"

static const char *TAG = "PYRO";
static adc_oneshot_unit_handle_t adc1_handle;
static adc_cali_handle_t adc_cali_handle[4];  // One calibration handle per channel

// Array to map channel numbers to GPIO pins
static const gpio_num_t pyro_out_pins[] = {
    PYRO1_OUT, PYRO2_OUT, PYRO3_OUT, PYRO4_OUT
};

static pyro_state_t pyro_state[5] = {0};

esp_err_t pyro_init(void) {
    // Configure output pins
    for (int i = 0; i < 4; i++) {
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << pyro_out_pins[i]),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_DISABLE
        };
        ESP_ERROR_CHECK(gpio_config(&io_conf));
        gpio_set_level(pyro_out_pins[i], 0);
    }

    // Initialize ADC
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc1_handle));

    // Initialize calibration for each channel
    for (int i = 0; i < 4; i++) {
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = ADC_UNIT_1,
            .chan = i,  // ADC_CHANNEL_0 through ADC_CHANNEL_3
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_DEFAULT
        };
        ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle[i]));
    }

    return ESP_OK;
}

bool pyro_continuity(pyro_channel_t channel) {
    if (channel < 1 || channel > 4) {
        return false;
    }

    // Get the corresponding ADC channel from the enum
    adc_channel_t adc_chan;
    switch (channel) {
        case PYRO_CHANNEL_1:
            adc_chan = ADC_CHANNEL_0;
            break;
        case PYRO_CHANNEL_2:
            adc_chan = ADC_CHANNEL_1;
            break;
        case PYRO_CHANNEL_3:
            adc_chan = ADC_CHANNEL_2;
            break;
        case PYRO_CHANNEL_4:
            adc_chan = ADC_CHANNEL_3;
            break;
        default:
            return false;
    }

    // Configure ADC channel for this reading
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, adc_chan, &channel_config));

    // Read ADC
    int adc_raw;
    int voltage_mv;
    esp_err_t ret = adc_oneshot_read(adc1_handle, adc_chan, &adc_raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC read error on channel %d", channel);
        return false;
    }

    // Convert raw reading to millivolts using calibration
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle[adc_chan], adc_raw, &voltage_mv));
    double voltage = voltage_mv / 1000.0;
    
    // Return true if voltage is above 1V threshold
    return (voltage > 1.0);
}

esp_err_t pyro_activate(pyro_channel_t channel, uint8_t delay, bool bypass) {
    if (channel < 1 || channel > 4) {
        return ESP_ERR_INVALID_ARG;
    }

    // Check continuity before activation
    if (!bypass & !pyro_continuity(channel)) {
        ESP_LOGE(TAG, "No continuity detected on channel %d", channel);
        return ESP_ERR_INVALID_STATE;
    }

    // Activate the pyro channel
    gpio_set_level(pyro_out_pins[channel - 1], 1);
    
    pyro_state[channel].start_us = esp_timer_get_time();
    pyro_state[channel].duration_ms = delay;
    pyro_state[channel].state = true;

    return ESP_OK;
}

esp_err_t pyro_deactivate(pyro_channel_t channel) {
    if (channel < 1 || channel > 4) {
        return ESP_ERR_INVALID_ARG;
    }
    gpio_set_level(pyro_out_pins[channel - 1], 0);
    pyro_state[channel].state = false;
    return ESP_OK;
}

esp_err_t pyro_update_state(void) {
    for (pyro_channel_t channel = PYRO_CHANNEL_1; channel <= PYRO_CHANNEL_4; channel++) {
        if (pyro_state[channel].state && pyro_state[channel].duration_ms != 0) {
            if (esp_timer_get_time() - pyro_state[channel].start_us > pyro_state[channel].duration_ms * 1000) {
                pyro_deactivate(channel);
            }
        }
    }
    return ESP_OK;
}

esp_err_t pyro_poll_state(pyro_state_t *state[5]) {
    *state = pyro_state;
    return ESP_OK;
}

void pyro_deinit(void) {
    // Clean up calibration handles
    for (int i = 0; i < 4; i++) {
        adc_cali_delete_scheme_curve_fitting(adc_cali_handle[i]);
    }
    adc_oneshot_del_unit(adc1_handle);
}
