#include "driver_psu.h"
#include "ascent_r3_hardware_definition.h"
#include "adc_oneshot.h"
#include "esp_log.h"
#include "adc_cali.h"
#include "adc_cali_scheme.h"
#include "driver/usb_serial_jtag.h"

static const char *TAG = "PSU";
static adc_oneshot_unit_handle_t adc2_handle;
static adc_cali_handle_t adc_cali_handle;
static power_source_t current_power_source = POWER_SOURCE_ERROR;
static battery_type_t current_battery_type = BATTERY_TYPE_NONE;
static const voltage_range_t BATTERY_RANGES[] = BATTERY_VOLTAGE_RANGES;

// Function prototypes for internal functions
static battery_type_t autodetect_battery_type(double voltage);
static const char* battery_type_to_string(battery_type_t type);
static const char* psu_mode_to_string(psu_mode_t mode);

esp_err_t psu_init_default(void) {
    return psu_init(BATTERY_TYPE_NONE);
}

esp_err_t psu_init(battery_type_t battery_type) {
    // Initialize ADC2
    adc_oneshot_unit_init_cfg_t adc_config = {
        .unit_id = ADC_UNIT_1,
        .ulp_mode = ADC_ULP_MODE_DISABLE,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&adc_config, &adc2_handle));

    // Set up ADC calibration
    adc_cali_curve_fitting_config_t cali_config = {
        .unit_id = ADC_UNIT_1,
        .chan = ADC_CHANNEL_6,
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &adc_cali_handle));

    // Set initial battery type
    if (battery_type == BATTERY_TYPE_NONE) {
        ESP_LOGI(TAG, "No battery type specified, attempting autodetection...");
        power_status_t status = psu_get_power_source();
        if (status.source != POWER_SOURCE_USB_ONLY && status.source != POWER_SOURCE_ERROR) {
            current_battery_type = autodetect_battery_type(psu_read_battery_voltage());
        } else {
            ESP_LOGI(TAG, "No battery detected, setting type to NONE");
            current_battery_type = BATTERY_TYPE_NONE;
        }
    } else {
        ESP_LOGI(TAG, "Using specified battery type: %s", battery_type_to_string(battery_type));
        current_battery_type = battery_type;
    }

    ESP_LOGI(TAG, "Initialization complete. Battery type: %s", battery_type_to_string(current_battery_type));
    return ESP_OK;
}

double psu_read_battery_voltage(void) {
    // Configure ADC channel for battery reading (GPIO17 = ADC2_CHANNEL_6)
    adc_oneshot_chan_cfg_t channel_config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc2_handle, ADC_CHANNEL_6, &channel_config));

    // Read ADC
    int adc_raw;
    int voltage_mv;
    esp_err_t ret = adc_oneshot_read(adc2_handle, ADC_CHANNEL_6, &adc_raw);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "ADC read error on battery voltage pin");
        return -1.0;
    }

    // Convert raw reading to millivolts using calibration
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc_cali_handle, adc_raw, &voltage_mv));
    
    // Convert from mV to V and apply voltage divider scaling
    double measured_voltage = voltage_mv / 1000.0;
    double actual_voltage = measured_voltage * DIVIDER_RATIO * MAGIC;

    return actual_voltage;
}

power_status_t psu_get_power_source(void) {
    power_status_t status = {
        .source = POWER_SOURCE_ERROR,
        .battery_type = current_battery_type,
        .battery_percentage = -1
    };

    bool usb_connected = usb_serial_jtag_is_connected();
    double battery_voltage = psu_read_battery_voltage();
    
    if (battery_voltage < 0) {
        return status;
    }
    
    if (usb_connected) {
        if (battery_voltage >= 4.0 && battery_voltage <= 4.5) {
            status.source = POWER_SOURCE_USB_ONLY;
        } else if (battery_voltage > 2.0) {
            status.source = POWER_SOURCE_BOTH;
            if (current_battery_type != BATTERY_TYPE_NONE) {
                status.battery_percentage = psu_get_battery_percentage();
            }
        }
    } else if (battery_voltage > 6.0) {
        status.source = POWER_SOURCE_BATTERY_ONLY;
        if (current_battery_type != BATTERY_TYPE_NONE) {
            status.battery_percentage = psu_get_battery_percentage();
        }
    }

    current_power_source = status.source;
    return status;
}

void psu_deinit(void) {
    adc_cali_delete_scheme_curve_fitting(adc_cali_handle);
    adc_oneshot_del_unit(adc2_handle);
}

static battery_type_t autodetect_battery_type(double voltage) {
    ESP_LOGI(TAG, "Autodetecting battery type for voltage: %.3fV", voltage);
    for (int i = 0; i < sizeof(BATTERY_RANGES)/sizeof(voltage_range_t); i++) {
        if (voltage >= BATTERY_RANGES[i].min_v && voltage <= BATTERY_RANGES[i].max_v) {
            ESP_LOGI(TAG, "Detected battery type: %s", battery_type_to_string((battery_type_t)i));
            return (battery_type_t)i;
        }
    }
    ESP_LOGW(TAG, "Could not determine battery type for voltage: %.3fV", voltage);
    return BATTERY_TYPE_UNKNOWN;
}

battery_type_t psu_autodetect_battery(void) {
    power_source_t power_source = psu_get_power_source().source;
    current_power_source = power_source;
    
    if (power_source == POWER_SOURCE_USB_ONLY || power_source == POWER_SOURCE_ERROR) {
        current_battery_type = BATTERY_TYPE_NONE;
        return BATTERY_TYPE_NONE;
    }
    
    double voltage = psu_read_battery_voltage();
    current_battery_type = autodetect_battery_type(voltage);
    return current_battery_type;
}

int psu_get_battery_percentage(void) {
    // If no battery type stored, run autodetect
    if (current_battery_type == BATTERY_TYPE_NONE) {
        psu_autodetect_battery();
    }
    
    // If still no battery or unknown type, return error
    if (current_battery_type == BATTERY_TYPE_NONE || 
        current_battery_type == BATTERY_TYPE_UNKNOWN) {
        return -1;
    }
    
    double voltage = psu_read_battery_voltage();
    double min_v = BATTERY_RANGES[current_battery_type].min_v;
    double max_v = BATTERY_RANGES[current_battery_type].max_v;
    
    // Calculate percentage
    int percentage = (int)(((voltage - min_v) / (max_v - min_v)) * 100.0);
    
    // Clamp percentage between 0 and 100
    if (percentage > 100) percentage = 100;
    if (percentage < 0) percentage = 0;
    
    return percentage;
}

battery_type_t psu_get_battery_type() {
    if (current_battery_type == BATTERY_TYPE_NONE && (current_power_source == POWER_SOURCE_BATTERY_ONLY || current_power_source == POWER_SOURCE_BOTH)) {
        ESP_LOGE(TAG, "Battery type not set, autodetecting...");
        psu_autodetect_battery();
    }
    return current_battery_type;
}

static const char* battery_type_to_string(battery_type_t type) {
    switch (type) {
        case BATTERY_TYPE_1S_LITHIUM:
            return "1S Lithium";
        case BATTERY_TYPE_2S_LITHIUM:
            return "2S Lithium";
        case BATTERY_TYPE_9V:
            return "9V Battery";
        case BATTERY_TYPE_3S_LITHIUM:
            return "3S Lithium";
        case BATTERY_TYPE_LICB_12V:
            return "LiCB 12V";
        case BATTERY_TYPE_2S_ALKALINE:
            return "2S Alkaline";
        case BATTERY_TYPE_UNKNOWN:
            return "Unknown";
        case BATTERY_TYPE_NONE:
            return "No Battery";
        default:
            return "Invalid";
    }
}

static const char* psu_mode_to_string(psu_mode_t mode) {
    switch (mode) {
        case PSU_MODE_BUCK:
            return "Buck Mode";
        case PSU_MODE_BOOST:
            return "Boost Mode";
        case PSU_MODE_PERFECT_SOURCE:
            return "Perfect Source";
        case PSU_MODE_ERROR:
        default:
            return "Error";
    }
}

psu_mode_t psu_get_mode(void) {
    double voltage = psu_read_battery_voltage();
    if (voltage < 0) {
        return PSU_MODE_ERROR;
    }

    const double TARGET_VOLTAGE = DIVIDER_RATIO;
    const double TOLERANCE = 0.1;

    if (voltage > TARGET_VOLTAGE + TOLERANCE) {
        ESP_LOGI(TAG, "Operating in Buck mode (Vin: %.3fV)", voltage);
        return PSU_MODE_BUCK;
    } else if (voltage < TARGET_VOLTAGE - TOLERANCE) {
        ESP_LOGI(TAG, "Operating in Boost mode (Vin: %.3fV)", voltage);
        return PSU_MODE_BOOST;
    } else {
        ESP_LOGI(TAG, "Operating in Perfect Source mode (Vin: %.3fV)", voltage);
        return PSU_MODE_PERFECT_SOURCE;
    }
}
