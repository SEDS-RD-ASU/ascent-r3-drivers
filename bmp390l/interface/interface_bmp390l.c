#include "interface_bmp390l.h"
#include "ascent_r3_hardware_definition.h"
#include "driver_BMP390L.h"
#include "math.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "i2c_manager.h"

static const char *TAG = "BMP390 INTERFACE";

// Static calibration parameters
static float bmp_scaling = 1.0f;  // Default to no scaling
static float bmp_bias = 0.0f;     // Default to no bias
static double ground_alt = 0.0;    // Ground altitude for local reference

// Interrupt callback
static void (*user_int_callback)(void *args) = NULL;
static void *user_int_args = NULL;

static void update_ground_pressure(double *groundPressure, double *groundTemperature, uint8_t num_readings);

static void pressure_to_m(double *pressure, double *temperature, double *alt) { // formula used: https://www.nakka-rocketry.net/apogee.html
    if (*pressure <= 0.0) {
        ESP_LOGE(TAG, "Invalid pressure input: %.2f", *pressure);
        *alt = 0.0;
        return;
    }

    // *alt = ((*temperature+273.15)/0.0065) * (1.0 - pow(*pressure / 1013.25, 1.0 / 5.255));
    *alt = ((25+273.15)/0.0065) * (1.0 - pow(*pressure / 1013.25, 1.0 / 5.255));
}

void bmp390_set_ground_alt(double new_ground_alt) {
    double groundPressure;
    double groundTemperature;

    if (new_ground_alt != 0.0) {
        ground_alt = new_ground_alt; // store new ground altitude in static variable
    } else {
        ESP_LOGW(TAG, "NO NEW GROUND REFERENCE ALTITUDE PASSED. READING NEW GROUND PRESSURE AND CONVERTING TO METERS.");
        update_ground_pressure(&groundPressure, &groundTemperature, 100);
        pressure_to_m(&groundPressure, &groundTemperature, &ground_alt);
        ESP_LOGW(TAG, "NEW GROUND ALTITUDE: %f METERS ABOVE SEA LEVEL", ground_alt);
    }
}

static void update_ground_pressure(double *groundPressure, double *groundTemperature, uint8_t num_readings) {
    *groundPressure = 1013.25; // Default ground pressure in hPa
    *groundTemperature = 25.0; // Default ground temperature in Celsius

    double totalPressure = 0.0;    // Initialize total pressure
    double totalTemperature = 0.0; // Initialize total temperature

    // Loop over the number of readings
    for (int i = 0; i < num_readings; i++) {
        double pressure, temperature; // Declare pressure and temperature variables

        // Read sensor data and store the return value
        esp_err_t ret = bmp390_read_sensor_data(&pressure, &temperature);

        // If the sensor data read is not successful, log an error and return
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read sensor data");
            return;
        }

        totalPressure += pressure;       // Add the pressure to the total pressure
        totalTemperature += temperature; // Add the temperature to the total temperature

        vTaskDelay(pdMS_TO_TICKS(30)); // Delay for 30 ms
    }

    // Calculate the average ground pressure and temperature
    *groundPressure = totalPressure / num_readings;  
    *groundTemperature = totalTemperature / num_readings;
}

void bmp390_set_bias(float new_scaling, float new_bias) {
    bmp_scaling = new_scaling;
    bmp_bias = new_bias;
}

static void bmp390_get_raw(baro_double_t* baro_out) {
    bmp390_read_sensor_data(&baro_out->pressure, &baro_out->temperature);
}

static void bmp390_correct_bias(baro_double_t* baro_out) {
    // Get raw data
    bmp390_get_raw(baro_out);
    // Apply scaling and bias correction
    baro_out->pressure = baro_out->pressure * bmp_scaling + bmp_bias;
}

void bmp390_get_local(baro_double_t* baro_out) {
    // Correct for bias
    bmp390_correct_bias(baro_out);
    // Convert pressure to meters
    pressure_to_m(&baro_out->pressure, &baro_out->temperature, &baro_out->alt);
    // Convert to altitude above ground level
    baro_out->alt = baro_out->alt - ground_alt;
}

// GPIO ISR handler for interrupts
static void IRAM_ATTR bmp390_isr_handler(void *args) {
    if (user_int_callback != NULL) {
        user_int_callback(user_int_args);
    }
}

esp_err_t bmp390_flight_init(i2c_port_t port, void int_cb(void *args), void *cb_args) {
    esp_err_t ret;
    // Initialize the BMP390 sensor
    ret = bmp390_init(port);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor!");
        return ret;
    }

    bmp390_osr_settings_t osr_settings = {
        .press_os = BMP390_OVERSAMPLING_1X,
        .temp_os = BMP390_OVERSAMPLING_1X
    };

    ret = bmp390_set_osr(&osr_settings);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set OSR!");
        return ret;
    }

    bmp390_odr_t odr_settings = BMP390_ODR_200HZ;

    ret = bmp390_set_odr(odr_settings);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set ODR!");
        return ret;
    }

    bmp390_config_t filterconfig = {
        .iir_filter = BMP390_IIR_FILTER_COEFF_31
    };

    bmp390_set_config(&filterconfig);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set filter config!");
        return ret;
    }

    bmp390_int_config_t intconfig = {
        1, // drdy_en: Enable Data Ready interrupt
        0, // fwtm_en: Disable FIFO watermark interrupt
        0, // ffull_en: Disable FIFO full interrupt
        0, // int_latch: Non-latching mode
        0, // int_od: Push-pull output
        1  // int_level: Active high
    };
    ret = bmp390_set_int_config(&intconfig);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set interrupt config!");
        return ret;
    }
    
    ESP_LOGI(TAG, "BMP Settings Configured!");

    vTaskDelay(pdMS_TO_TICKS(10));
    
    bmp390_set_ground_alt(0); // automatically calcualte ground altitude

    // Configure interrupt callback (and configure GPIO)
    if (int_cb != NULL)
    {
        user_int_callback = int_cb;
        user_int_args = cb_args;

        // Configure GPIO for interrupt
        gpio_config_t io_conf = {
            .pin_bit_mask = (1ULL << PIN_BMP390_INT),
            .mode = GPIO_MODE_INPUT,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
            .intr_type = GPIO_INTR_POSEDGE
        };

        ret = gpio_config(&io_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to configure GPIO for interrupt");
            return ret;
        }

        // Add ISR handler
        ret = gpio_isr_handler_add(PIN_BMP390_INT, bmp390_isr_handler, NULL);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to add ISR handler");
            return ret;
        }
        ESP_LOGI(TAG, "BMP390 Interrupt Configured on GPIO %d", PIN_BMP390_INT);
    }

    return ESP_OK;
}

double bmp390_ground_altitude(void)
{
    return ground_alt;
}