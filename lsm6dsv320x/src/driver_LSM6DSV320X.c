#include "driver_LSM6DSV320X.h"

static const char *TAG = "LSM6DSV320X DRIVER";

#define MAX_TRANSACTION_SIZE 8192
#define NOTHING 0x00

static spi_device_handle_t lsm_handle;
static uint8_t g_transaction_buf[MAX_TRANSACTION_SIZE];

static spi_device_interface_config_t lsm_cfg = {
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

static esp_err_t lsm_write_register(uint8_t reg, uint8_t value)
{
    memset(g_transaction_buf, NOTHING, MAX_TRANSACTION_SIZE); // clear out buffer
    
    g_transaction_buf[0] = reg & 0x7F; // register with write bit set
    g_transaction_buf[1] = value; // what is being written to the register

    spi_transaction_t cmd = {
        .length = 16, // register address + 1 byte
        .tx_buffer = g_transaction_buf,
        .rx_buffer = g_transaction_buf, // does nothing
    };

    esp_err_t err = spi_device_polling_transmit(lsm_handle, &cmd);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "IMU SPI transaction failed");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t lsm_get_who_am_i(void)
{
    uint8_t res;
    
    esp_err_t spi_ret = lsm_read_multiple(0x0F, 1, &res);
    if(spi_ret) return spi_ret;

    if (res != 0x73) {
        ESP_LOGE(TAG, "WHO_AM_I mismatch! Got 0x%02X, expected 0x73", res);
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t lsm_reset(void)
{
    esp_err_t ret = lsm_write_register(LSM6DSV320X_CTRL3, 0x01);
    if(ret) return ret;

    vTaskDelay(pdMS_TO_TICKS(15)); // allow device to reboot

    ret = lsm_get_who_am_i(); // has the device stopped identifying itself?
    //todo: find better way of validating a successful reset
    if(ret) return ret;

    return ESP_OK;
}

static esp_err_t lsm_enable_bdu(void) // by default this is already enabled. but I am paranoid.
{
    uint8_t ctrl3;

    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL3, 1, &ctrl3);
    if(ret) return ret;
    
    ctrl3 |= 0x40;
    
    ret = lsm_write_register(LSM6DSV320X_CTRL3, ctrl3);
    if(ret) return ret;

    return ESP_OK;
}

esp_err_t lsm_init(spi_host_device_t host)
{
    esp_err_t ret;

    ret = spi_host_initialized(host);
    if(ret){
        ESP_LOGE(TAG, "SPI HOST NOT INITIALIZED");
        return ESP_FAIL;
    }

    ret = spi_bus_add_device(host, &lsm_cfg,&lsm_handle);
    if(ret){
        ESP_LOGE(TAG, "FAILED TO ADD LSM TO SPI BUS");
        return ret;
    }

    // Small delay to ensure device is ready
    vTaskDelay(pdMS_TO_TICKS(10));

    ret = lsm_reset();
    if(ret){
        ESP_LOGE(TAG, "FAILED TO RESET LSM");
        return ret;
    }

    ret = lsm_get_who_am_i();
    if(ret){
        ESP_LOGE(TAG, "FAILED TO GET LSM DEVICE ID");
        return ret;
    }

    ret = lsm_enable_bdu();
    if(ret){
        ESP_LOGE(TAG, "FAILED TO ENABLE LSM BDU");
        return ret;
    }
    
    ESP_LOGI(TAG, "LSM6DSV320X initialization successful!");

    return ESP_OK;
}

esp_err_t lsm_set_lowgacc_odr(lsm6dsv320x_data_rate_t odr)
{
    uint8_t ctrl1;
    uint8_t haodr;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL1, 1, &ctrl1);
    if(ret) return ret;

    ctrl1 |= (odr & 0x0F);

    ret = lsm_write_register(LSM6DSV320X_CTRL1, ctrl1);
    if(ret) return ret;

    if((odr >> 4) & 0xF) // if this is a HAODR mode, update the register for HAODR config as well
    {
        ret = lsm_read_multiple(LSM6DSV320X_HAODR_CFG, 1, &haodr);
        if(ret) return ret;

        haodr |= ((odr >> 4) & 0xF);

        ret = lsm_write_register(LSM6DSV320X_HAODR_CFG, haodr);
        if(ret) return ret;
    }
    
    return ESP_OK;
}

esp_err_t lsm_set_highgacc_odr(lsm6dsv320x_hg_xl_data_rate_t odr)
{
    uint8_t ctrl1_xl_hg;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL1_XL_HG, 1, &ctrl1_xl_hg);
    if(ret) return ret;

    ctrl1_xl_hg |= (odr << 3);
    ctrl1_xl_hg |= 0x80; // enable high-g accelerometer

    ret = lsm_write_register(LSM6DSV320X_CTRL1_XL_HG, ctrl1_xl_hg);
    if(ret) return ret;
    
    return ESP_OK;
}

esp_err_t lsm_set_lowgacc_mode(lsm6dsv320x_xl_mode_t mode)
{
    uint8_t ctrl1;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL1, 1, &ctrl1);
    if(ret) return ret;

    ctrl1 |= (mode << 4);

    ret = lsm_write_register(LSM6DSV320X_CTRL1, ctrl1);
    if(ret) return ret;
    
    return ESP_OK;
}

esp_err_t lsm_set_gyr_odr(lsm6dsv320x_data_rate_t odr)
{
    uint8_t ctrl2;
    uint8_t haodr;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL2, 1, &ctrl2);
    if(ret) return ret;

    ctrl2 |= odr;

    ret = lsm_write_register(LSM6DSV320X_CTRL2, ctrl2);
    if(ret) return ret;

    if((odr >> 4) & 0xF) // if this is a HAODR mode, update the register for HAODR config as well
    {
        ret = lsm_read_multiple(LSM6DSV320X_HAODR_CFG, 1, &haodr);
        if(ret) return ret;

        haodr |= ((odr >> 4) & 0xF);

        ret = lsm_write_register(LSM6DSV320X_HAODR_CFG, haodr);
        if(ret) return ret;
    }

    return ESP_OK;
}

esp_err_t lsm_set_gyr_mode(lsm6dsv320x_gy_mode_t mode)
{
    uint8_t ctrl2;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL2, 1, &ctrl2);
    if(ret) return ret;

    ctrl2 |= (mode << 4);

    ret = lsm_write_register(LSM6DSV320X_CTRL2, ctrl2);
    if(ret) return ret;

    return ESP_OK;
}

esp_err_t lsm_set_gyr_scale(lsm6dsv320x_gy_full_scale_t scale)
{
    uint8_t ctrl6;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL6, 1, &ctrl6);
    if(ret) return ret;

    ctrl6 |= scale;

    ret = lsm_write_register(LSM6DSV320X_CTRL6, ctrl6);
    if(ret) return ret;

    return ESP_OK;
}

esp_err_t lsm_set_lowgacc_scale(lsm6dsv320x_xl_full_scale_t scale)
{
    uint8_t ctrl8;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL8, 1, &ctrl8);
    if(ret) return ret;

    ctrl8 |= scale;

    ret = lsm_write_register(LSM6DSV320X_CTRL8, ctrl8);
    if(ret) return ret;
    
    return ESP_OK;
}

esp_err_t lsm_set_highgacc_scale(lsm6dsv320x_hg_xl_full_scale_t scale)
{
    uint8_t ctrl1_xl_hg;
    esp_err_t ret;

    ret = lsm_read_multiple(LSM6DSV320X_CTRL1_XL_HG, 1, &ctrl1_xl_hg);
    if(ret) return ret;

    ctrl1_xl_hg |= scale;
    ctrl1_xl_hg |= 0x80; // enable high-g accelerometer

    ret = lsm_write_register(LSM6DSV320X_CTRL1_XL_HG, ctrl1_xl_hg);
    if(ret) return ret;
    
    return ESP_OK;
}

esp_err_t lsm_get_raw(lsm_raw_data_t *raw_imu_data)
{
    uint8_t raw_data_buffer[2];

    lsm_read_multiple(LSM6DSV320X_OUT_TEMP_L, 2, raw_data_buffer);

    raw_imu_data->temp = (float)(int16_t)((raw_data_buffer[1] << 8) | raw_data_buffer[0]) / 256.0f + 25.0f;  // LSM6DSV320X temperature conversion

    return ESP_OK;
}