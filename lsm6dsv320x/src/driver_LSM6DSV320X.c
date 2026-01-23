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

static uint8_t spi_write_read(uint8_t *in_buf, uint32_t in_len, uint8_t *out_buf, uint32_t out_len)
{

    memset(g_transaction_buf, NOTHING, MAX_TRANSACTION_SIZE);
    
    memcpy(g_transaction_buf, in_buf, in_len);

    spi_transaction_t cmd = {
        .length = (in_len+out_len)*8,
        .tx_buffer = g_transaction_buf,
        .rx_buffer = g_transaction_buf,
    };

    esp_err_t err = spi_device_polling_transmit(lsm_handle, &cmd);
    if (err != ESP_OK) {
        printf("IMU SPI transaction failed\n");
        return 1;
    }

    memcpy(out_buf, g_transaction_buf+in_len, out_len);

    return 0;
}

esp_err_t lsm_get_who_am_i(uint8_t *out)
{
    uint8_t cmd[] = { 0x8F };  // Read WHO_AM_I register
    uint8_t res[1];
    
    ESP_LOGI(TAG, "Sending WHO_AM_I command: 0x%02X", cmd[0]);
    
    uint8_t spi_res = spi_write_read(cmd, sizeof(cmd), res, sizeof(res));
    if (spi_res) {
        ESP_LOGE(TAG, "SPI transaction failed");
        return spi_res;
    }

    ESP_LOGI(TAG, "Raw WHO_AM_I response: 0x%02X", res[0]);
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

    ESP_LOGI(TAG, "WHO_AM_I: 0x%02X (expected 0x73)", who_am_i);
    
    if (who_am_i != 0x73) {
        ESP_LOGE(TAG, "WHO_AM_I mismatch! Got 0x%02X, expected 0x73", who_am_i);
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "LSM6DSV320X initialization successful");

    return ESP_OK;
}