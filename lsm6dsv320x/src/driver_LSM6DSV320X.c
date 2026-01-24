#include "driver_LSM6DSV320X.h"

static const char *TAG = "LSM6DSV320X DRIVER";

#define MAX_TRANSACTION_SIZE 8192
#define NOTHING 0x00

static spi_device_handle_t lsm_handle;
static uint8_t g_transaction_buf[MAX_TRANSACTION_SIZE];

spi_device_interface_config_t lsm_cfg = {
    .mode = 3,
    .clock_speed_hz = 1e6,
    .spics_io_num = IMU_CS,
    .queue_size = 1,
};


static esp_err_t lsm_read_multiple(uint8_t reg, uint8_t num_bytes, uint8_t *out_buf)
{
    if (num_bytes + 1 > MAX_TRANSACTION_SIZE) {
        return ESP_ERR_INVALID_SIZE;
    }

    memset(g_transaction_buf, NOTHING, MAX_TRANSACTION_SIZE); // clear out buffer
    
    g_transaction_buf[0] = reg | 0x80; // register with read bit set

    spi_transaction_t cmd = {
        .length = (1+num_bytes)*8, // bits to transfer
        .tx_buffer = g_transaction_buf,
        .rx_buffer = g_transaction_buf, // received bytes will overwrite the tx buffer
    };

    esp_err_t err = spi_device_polling_transmit(lsm_handle, &cmd);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "IMU SPI transaction failed");
        return ESP_FAIL;
    }

    memcpy(out_buf, g_transaction_buf+1, num_bytes); // copy bytes from received data, skipping what's received during the command phase

    return ESP_OK;
}

static esp_err_t lsm_get_who_am_i(uint8_t *out)
{
    uint8_t res[1];
    
    esp_err_t spi_ret = lsm_read_multiple(0x0F, 1, res);
    if (spi_ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transaction failed");
        return spi_ret;
    }

    memcpy(out, res, sizeof(res));

    return 0;
}

esp_err_t lsm_init(spi_host_device_t host)
{
    esp_err_t ret;

    if(spi_host_initialized(host) != ESP_OK){
        ESP_LOGE(TAG, "SPI HOST NOT INITIALIZED");
        return ESP_FAIL;
    }

    ret = spi_bus_add_device(host, &lsm_cfg,&lsm_handle);
    if(ret != ESP_OK){
        ESP_LOGE(TAG, "FAILED TO ADD LSM TO SPI BUS");
        return ret;
    }

    // Small delay to ensure device is ready
    vTaskDelay(pdMS_TO_TICKS(10));

    uint8_t who_am_i;
    ret = lsm_get_who_am_i(&who_am_i);
    
    if (who_am_i != 0x73) {
        ESP_LOGE(TAG, "WHO_AM_I mismatch! Got 0x%02X, expected 0x73", who_am_i);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "LSM6DSV320X initialization successful!");

    return ESP_OK;
}