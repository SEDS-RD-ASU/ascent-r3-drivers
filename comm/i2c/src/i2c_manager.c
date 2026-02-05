#include "i2c_manager.h"
#include <stdbool.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static const char *TAG = "I2C MANAGER";

// Static array to track initialization status of I2C ports
static bool i2c_initialized[I2C_NUM_MAX] = {false};

// Mutex for thread-safe I2C access (one per port)
static SemaphoreHandle_t i2c_mutex[I2C_NUM_MAX] = {NULL};

esp_err_t i2c_manager_init(int sda_pin, int scl_pin, uint32_t freq_hz, i2c_port_t port) {
    // Check if already initialized
    if (i2c_initialized[port]) {
        return ESP_OK; // Already initialized, just return success
    }

    // Create mutex for this port
    if (i2c_mutex[port] == NULL) {
        i2c_mutex[port] = xSemaphoreCreateMutex();
        if (i2c_mutex[port] == NULL) {
            ESP_LOGE(TAG, "Failed to create I2C mutex for port %d", port);
            return ESP_ERR_NO_MEM;
        }
    }

    // Configure I2C
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = sda_pin,
        .scl_io_num = scl_pin,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = freq_hz,
    };

    esp_err_t ret = i2c_param_config(port, &conf);
    if (ret != ESP_OK) {
        return ret;
    }

    // Use ESP_INTR_FLAG_IRAM to ensure ISR is in IRAM (safer with BLE)
    // and ESP_INTR_FLAG_LOWMED to use lower priority interrupt (avoids conflicts)
    ret = i2c_driver_install(port, I2C_MODE_MASTER, 0, 0, ESP_INTR_FLAG_IRAM | ESP_INTR_FLAG_LOWMED);
    if (ret != ESP_OK) {
        return ret;
    }

    i2c_initialized[port] = true;
    return ESP_OK;
}

bool i2c_manager_is_initialized(i2c_port_t port) {
    if (port >= I2C_NUM_MAX) {
        return false;
    }
    return i2c_initialized[port];
}

esp_err_t i2c_manager_deinit(i2c_port_t port) {
    if (!i2c_initialized[port]) {
        return ESP_OK; // Already deinitialized
    }

    esp_err_t ret = i2c_driver_delete(port);
    if (ret == ESP_OK) {
        i2c_initialized[port] = false;
    }
    return ret;
}

esp_err_t i2c_manager_read_register(i2c_port_t port, uint8_t device_addr, 
                                  uint8_t reg, uint8_t *data, size_t len) {
    if (!i2c_initialized[port]) {
        return ESP_ERR_INVALID_STATE;
    }

    // Take mutex before I2C operation
    if (xSemaphoreTake(i2c_mutex[port], pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for read_register");
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Write the register address we want to read from
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    
    // Perform repeated start and read the data
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    xSemaphoreGive(i2c_mutex[port]);  // Release mutex
    
    if (ret != ESP_OK) {
        printf("i2c_manager_read_register failed with error code: %d\n", ret);
    }
    
    return ret;
}

esp_err_t i2c_manager_write_register(i2c_port_t port, uint8_t device_addr, 
                                   uint8_t reg, uint8_t *data, size_t len) {
    if (!i2c_initialized[port]) {
        return ESP_ERR_INVALID_STATE;
    }

    // Take mutex before I2C operation
    if (xSemaphoreTake(i2c_mutex[port], pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for write_register");
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    xSemaphoreGive(i2c_mutex[port]);  // Release mutex
    
    return ret;
} 

esp_err_t i2c_manager_write_yeet(i2c_port_t port, uint8_t device_addr, uint8_t *data, size_t len) {
    if (!i2c_initialized[port]) {
        return ESP_ERR_INVALID_STATE;
    }

    // Take mutex before I2C operation
    if (xSemaphoreTake(i2c_mutex[port], pdMS_TO_TICKS(1000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for write_yeet");
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true); 
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, 10000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);

    xSemaphoreGive(i2c_mutex[port]);  // Release mutex

    if (ret != ESP_OK) {
        printf("i2c_manager_write_yeet failed with error code: %d\n", ret);
    }
    
    return ret;
}

esp_err_t i2c_manager_read_yeet(i2c_port_t port, uint8_t device_addr, uint8_t *data, size_t len) {
    if (!i2c_initialized[port]) {
        return ESP_ERR_INVALID_STATE;
    }

    // Take mutex before I2C operation
    if (xSemaphoreTake(i2c_mutex[port], pdMS_TO_TICKS(2000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to take I2C mutex for read_yeet");
        return ESP_ERR_TIMEOUT;
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    // Write the register address we want to read from
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (device_addr << 1) | I2C_MASTER_READ, true);
    
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    esp_err_t ret = i2c_master_cmd_begin(port, cmd, 2000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    xSemaphoreGive(i2c_mutex[port]);  // Release mutex
    
    return ret;
}

// Copyright (c) 2021 Ruslan V. Uss
// Source: https://github.com/UncleRus/esp-idf-i2cscan/blob/main/main/main.c
esp_err_t i2c_scan(i2c_port_t port)
{
    ESP_LOGI(TAG, "PERFOMING I2C SCAN ON PORT %d", port);
    
    if (!i2c_initialized[port]) {
        return ESP_ERR_INVALID_STATE;
    }

    esp_err_t res;
    for (uint8_t i = 3; i < 0x78; i++)
    {
        i2c_cmd_handle_t cmd = i2c_cmd_link_create();
        i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
        i2c_master_stop(cmd);

        res = i2c_master_cmd_begin(port, cmd, 10 / portTICK_PERIOD_MS);
        if (res == 0){
            ESP_LOGI(TAG, "Device found at 0x%.2x", i);
        }
        i2c_cmd_link_delete(cmd);
    }
    vTaskDelay(pdMS_TO_TICKS(1000));

	ESP_LOGI(TAG, "I2C Scan Finished!");

    return ESP_OK;
}

esp_err_t i2c_flight_init(void)
{
    esp_err_t ret;


    ret = i2c_manager_init(R3_SDA0,R3_SCL0,I2C_MASTER_FREQ_HZ,R3_I2C0_PORT);
    if(ret != ESP_OK) {ESP_LOGE(TAG, "FAILED TO INITIALIZE R3 I2C0"); return ret;}
    ret = i2c_manager_init(R3_SDA1,R3_SCL1,I2C_MASTER_FREQ_HZ,R3_I2C1_PORT);
    if(ret != ESP_OK) {ESP_LOGE(TAG, "FAILED TO INITIALIZE R3 I2C1"); return ret;}

    ESP_LOGI(TAG, "SUCCESSFULLY INITIALIZED ALL I2C BUSSES");

    i2c_scan(R3_I2C0_PORT);
    i2c_scan(R3_I2C1_PORT);

    return ESP_OK;
}