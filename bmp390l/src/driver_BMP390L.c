/**
 * @file driver_BMP390L.c
 * @brief BMP390L I2C Barometer Sensor Driver
 *
 * This file contains the implementation of all driver functions for the BMP390
 * pressure and temperature sensor, including:
 * - Sensor initialization and configuration
 * - Raw data reading and compensation
 * - Register access functions
 * - Calibration data handling
 */

#include <stdio.h>
#include <inttypes.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver_BMP390L.h"
#include "i2c_manager.h"

static const char *TAG = "BMP390 DRIVER";

static i2c_port_t i2c_port; // Port that the sensor is initialized on

static bmp390_calib_data_t calib_data;

// BMP390 register address definitions
#define BMP390_REG_CHIP_ID    0x00  // Chip ID register
#define BMP390_REG_PWR_CTRL   0x1B  // Power control register
#define BMP390_REG_OSR        0x1C  // Oversampling register
#define BMP390_REG_ODR        0x1D  // Output data rate register
#define BMP390_REG_CONFIG     0x1F  // Configuration register
#define BMP390_REG_PRESS_DATA 0x04  // Pressure data register
#define BMP390_REG_TEMP_DATA  0x07  // Temperature data register
#define BMP390_REG_ERR_REG    0x02  // Error register
#define BMP390_REG_STATUS     0x03  // Status register
#define BMP390_REG_INT_CTRL   0x19  // Interrupt control register

#define BMP390_STATUS_CMD_RDY 0x10
#define BMP390_I2C_ADDR       0x76

/*--- Read/Write to registers ---*/
static esp_err_t bmp390_read_register(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_manager_read_register(i2c_port, BMP390_I2C_ADDR, reg, data, len);
}

static esp_err_t bmp390_write_register(uint8_t reg, uint8_t *data, size_t len) {
    return i2c_manager_write_register(i2c_port, BMP390_I2C_ADDR, reg, data, len);
}

/*--- Function Implementations ---*/

/* Read chip ID from register 0x00 */
esp_err_t bmp390_get_chip_id(uint8_t *chip_id)
{
    return bmp390_read_register(BMP390_REG_CHIP_ID, chip_id, 1);
}

/* Read revision ID from register 0x01 */
esp_err_t bmp390_get_rev_id(uint8_t *rev_id) {
    return bmp390_read_register(0x01, rev_id, 1);
}


/* Get PWR_CTRL settings from register 0x1B */
esp_err_t bmp390_get_pwr_ctrl(bmp390_pwr_ctrl_t *ctrl)
{
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_PWR_CTRL, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    ctrl->press_en = (reg & 0x01) ? true : false;   // Bit0
    ctrl->temp_en  = (reg & 0x02) ? true : false;   // Bit1
    ctrl->mode     = (bmp390_mode_t)((reg & 0x30) >> 4);  // Bits 4-5
    return ESP_OK;
}

/* Set PWR_CTRL settings to register 0x1B */
esp_err_t bmp390_set_pwr_ctrl(const bmp390_pwr_ctrl_t *ctrl)
{
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_PWR_CTRL, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    // Set or clear pressure enable (bit0)
    if (ctrl->press_en)
        reg |= 0x01;
    else
        reg &= ~0x01;

    // Set or clear temperature enable (bit1)
    if (ctrl->temp_en)
        reg |= 0x02;
    else
        reg &= ~0x02;

    // Clear current mode bits (bits 4-5) and set new mode
    reg &= ~0x30;
    reg |= ((uint8_t)ctrl->mode << 4) & 0x30;

    return bmp390_write_register(BMP390_REG_PWR_CTRL, &reg, 1);
}

/* Get OSR settings from register 0x1C */
esp_err_t bmp390_get_osr(bmp390_osr_settings_t *osr)
{
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_OSR, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    osr->press_os = (bmp390_osr_t)(reg & 0x07);         // Bits [2:0]
    osr->temp_os  = (bmp390_osr_t)((reg & 0x38) >> 3);    // Bits [5:3]
    return ESP_OK;
}

/* Set OSR settings to register 0x1C */
esp_err_t bmp390_set_osr(const bmp390_osr_settings_t *osr)
{
    uint8_t reg = (((uint8_t)osr->temp_os << 3) & 0x38) | ((uint8_t)osr->press_os & 0x07);
    return bmp390_write_register(BMP390_REG_OSR, &reg, 1);
}

/* Get ODR setting from register 0x1D */
esp_err_t bmp390_get_odr(bmp390_odr_t *odr)
{
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_ODR, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    *odr = (bmp390_odr_t)(reg & 0x1F);  // Bits [4:0]
    return ESP_OK;
}

/* Set ODR setting to register 0x1D */
esp_err_t bmp390_set_odr(bmp390_odr_t odr)
{
    uint8_t reg = odr & 0x1F;
    return bmp390_write_register(BMP390_REG_ODR, &reg, 1);
}

/* Get CONFIG settings from register 0x1F */
esp_err_t bmp390_get_config(bmp390_config_t *config)
{
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_CONFIG, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    // Bits [3:1] hold the IIR filter coefficient.
    config->iir_filter = (bmp390_iir_filter_t)((reg & 0x0E) >> 1);
    return ESP_OK;
}

/* Set CONFIG settings to register 0x1F */
esp_err_t bmp390_set_config(const bmp390_config_t *config)
{
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_CONFIG, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    reg &= ~0x0E;  // Clear bits [3:1]
    reg |= (((uint8_t)config->iir_filter << 1) & 0x0E);
    return bmp390_write_register(BMP390_REG_CONFIG, &reg, 1);
}

/* Get error register (ERR_REG, 0x02) */
esp_err_t bmp390_get_err_reg(uint8_t *err)
{
    return bmp390_read_register(BMP390_REG_ERR_REG, err, 1);
}

/* Get status from register 0x03; we decode the CMD_RDY bit */
esp_err_t bmp390_get_status(bmp390_status_t *status)
{
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_STATUS, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    status->cmd_rdy = (reg & BMP390_STATUS_CMD_RDY) ? true : false;
    return ESP_OK;
}

// Replace existing compensation functions with these new versions
static int8_t bmp390_compensate_temperature(double *temperature,
                                     const bmp3_uncomp_data_t *uncomp_data,
                                     bmp390_calib_data_t *calib) {
    // Manufacturer API scaling
    double q_par_t1 = (double)calib->par_t1 * 256.0;
    double q_par_t2 = (double)calib->par_t2 / 1073741824.0;
    double q_par_t3 = (double)calib->par_t3 / 281474976710656.0;
    
    double uncomp_temp = (double)uncomp_data->temperature;
    double partial_data1 = uncomp_temp - q_par_t1;
    double partial_data2 = partial_data1 * q_par_t2;
    calib->t_lin = partial_data2 + (partial_data1 * partial_data1) * q_par_t3;
    *temperature = calib->t_lin;
    return BMP3_OK;
}

static int8_t bmp390_compensate_pressure(double *pressure,
                                  const bmp3_uncomp_data_t *uncomp_data,
                                  bmp390_calib_data_t *calib) {
    double uncomp_press = (double)uncomp_data->pressure;
    // Manufacturer API scaling for pressure coefficients
    double q_par_p1  = (((double)calib->par_p1) - 16384.0) / 1048576.0;
    double q_par_p2  = (((double)calib->par_p2) - 16384.0) / 536870912.0;
    double q_par_p3  = (double)calib->par_p3 / 4294967296.0;
    double q_par_p4  = (double)calib->par_p4 / 137438953472.0;
    double q_par_p5  = (double)calib->par_p5 / 0.125;  // Multiply by 8
    double q_par_p6  = (double)calib->par_p6 / 64.0;
    double q_par_p7  = (double)calib->par_p7 / 256.0;
    double q_par_p8  = (double)calib->par_p8 / 32768.0;
    double q_par_p9  = (double)calib->par_p9 / 281474976710656.0;
    double q_par_p10 = (double)calib->par_p10 / 281474976710656.0;
    double q_par_p11 = (double)calib->par_p11 / 36893488147419103232.0;
    
    double t_lin = calib->t_lin;  // already computed in temperature compensation

    double partial_data1 = q_par_p6 * t_lin;
    double partial_data2 = q_par_p7 * (t_lin * t_lin);
    double partial_data3 = q_par_p8 * (t_lin * t_lin * t_lin);
    double partial_out1 = q_par_p5 + partial_data1 + partial_data2 + partial_data3;

    partial_data1 = q_par_p2 * t_lin;
    double partial_data2b = q_par_p3 * (t_lin * t_lin);
    double partial_data3b = q_par_p4 * (t_lin * t_lin * t_lin);
    double partial_out2 = uncomp_press * (q_par_p1 + partial_data1 + partial_data2b + partial_data3b);

    double partial_data7 = uncomp_press * uncomp_press;
    double partial_data8 = q_par_p9 + q_par_p10 * t_lin;
    double partial_data9 = partial_data7 * partial_data8;
    double partial_data10 = partial_data9 + (uncomp_press * uncomp_press * uncomp_press) * q_par_p11;

    *pressure = partial_out1 + partial_out2 + partial_data10;
    return BMP3_OK;
}

// Replace the existing my_compensate_data function
static int8_t bmp390_compensate_data(uint8_t sensor_comp,
                              const bmp3_uncomp_data_t *uncomp_data,
                              bmp3_data_t *comp_data,
                              bmp390_calib_data_t *calib) {
    int8_t rslt = BMP3_OK;
    if (sensor_comp == BMP3_PRESS_TEMP) {
        rslt = bmp390_compensate_temperature(&comp_data->temperature, uncomp_data, calib);
        if (rslt == BMP3_OK)
            rslt = bmp390_compensate_pressure(&comp_data->pressure, uncomp_data, calib);
    }
    else if (sensor_comp == BMP3_PRESS) {
        (void)bmp390_compensate_temperature(&comp_data->temperature, uncomp_data, calib);
        comp_data->temperature = 0;
        rslt = bmp390_compensate_pressure(&comp_data->pressure, uncomp_data, calib);
    }
    else if (sensor_comp == BMP3_TEMP) {
        rslt = bmp390_compensate_temperature(&comp_data->temperature, uncomp_data, calib);
        comp_data->pressure = 0;
    }
    else {
        comp_data->pressure = 0;
        comp_data->temperature = 0;
    }
    return rslt;
}

static void bmp390_parse_sensor_data(const uint8_t *reg_data, bmp3_uncomp_data_t *uncomp_data) {
    uncomp_data->pressure    = ((uint32_t)reg_data[2] << 16) | ((uint32_t)reg_data[1] << 8) | reg_data[0];
    uncomp_data->temperature = ((uint32_t)reg_data[5] << 16) | ((uint32_t)reg_data[4] << 8) | reg_data[3];
}

// Replace bmp390_read_sensor_data with the new my_bmp390_get_sensor_data
esp_err_t bmp390_read_sensor_data(double *pressure, double *temperature) {
    uint8_t data[6];
    esp_err_t ret;

    // Read 6 bytes starting at BMP390_REG_PRESS_DATA
    ret = bmp390_read_register(BMP390_REG_PRESS_DATA, data, 6);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read sensor data");
        return ret;
    }

    bmp3_uncomp_data_t uncomp_data;
    bmp390_parse_sensor_data(data, &uncomp_data);

    bmp3_data_t comp_data;
    int8_t rslt = bmp390_compensate_data(BMP3_PRESS_TEMP, &uncomp_data, &comp_data, &calib_data);
    if (rslt != BMP3_OK) {
        ESP_LOGE(TAG, "Compensation error: %d", rslt);
        return ESP_FAIL;
    }

    // Return temperature in °C and pressure in hPa
    *temperature = comp_data.temperature;
    *pressure = comp_data.pressure / 100.0; // convert Pa to hPa
    return ESP_OK;
}

// Read calibration data from the sensor (registers 0x31 to 0x45, 21 bytes)  
esp_err_t bmp390_read_calib_data(bmp390_calib_data_t *calib) {
    uint8_t calib_buf[21];
    esp_err_t ret = bmp390_read_register(0x31, calib_buf, 21);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read calibration data");
        return ret;
    }

    // Parse calibration coefficients from calib_buf.
    calib->par_t1 = ((uint16_t)calib_buf[1] << 8) | calib_buf[0];
    calib->par_t2 = ((int16_t)calib_buf[3] << 8) | calib_buf[2];
    calib->par_t3 = (int8_t)calib_buf[4];

    calib->par_p1 = ((uint16_t)calib_buf[6] << 8) | calib_buf[5];
    calib->par_p2 = ((int16_t)calib_buf[8] << 8) | calib_buf[7];
    calib->par_p3 = (int8_t)calib_buf[9];
    calib->par_p4 = (int8_t)calib_buf[10];
    calib->par_p5 = ((uint16_t)calib_buf[12] << 8) | calib_buf[11];
    calib->par_p6 = ((uint16_t)calib_buf[14] << 8) | calib_buf[13];
    calib->par_p7 = (int8_t)calib_buf[15];
    calib->par_p8 = (int8_t)calib_buf[16];
    calib->par_p9 = ((int16_t)calib_buf[18] << 8) | calib_buf[17];
    calib->par_p10 = (int8_t)calib_buf[19];
    calib->par_p11 = (int8_t)calib_buf[20];

    return ESP_OK;
}

// bmp390_init: Initialize I2C, check chip ID, and read calibration data
esp_err_t bmp390_init(i2c_port_t port) {
    // Store only the port number
    i2c_port = port;
    
    // Check if I2C is already initialized
    if (!i2c_manager_is_initialized(port)) {
        ESP_LOGE(TAG, "I2C port %d not initialized", port);
        return ESP_ERR_INVALID_STATE;
    }

    // Verify chip ID (expecting 0x60 for BMP390)
    uint8_t chip_id;
    esp_err_t ret = bmp390_read_register(BMP390_REG_CHIP_ID, &chip_id, 1);
    if (ret != ESP_OK || chip_id != 0x60) {
        ESP_LOGE(TAG, "Failed to read chip ID or incorrect chip ID (0x%x)", chip_id);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "BMP390 initialized successfully with chip ID: 0x%x", chip_id);

    // Read calibration data once during initialization
    ret = bmp390_read_calib_data(&calib_data);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read calibration data during init");
        return ret;
    }

    // Enable sensors
    uint8_t pwr_ctrl = 0x33;
    ret = bmp390_write_register(BMP390_REG_PWR_CTRL, &pwr_ctrl, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable sensors");
        return ret;
    }

    return ESP_OK;
}



/**
 * @brief Get current interrupt configuration from INT_CTRL register
 * 
 * Reads the current interrupt settings from the sensor's INT_CTRL register
 * and decodes them into the configuration structure.
 *
 * @param[out] int_config Pointer to structure to store the configuration
 * @return ESP_OK if successful, error code otherwise
 */
esp_err_t bmp390_get_int_config(bmp390_int_config_t *int_config) {
    uint8_t reg;
    esp_err_t ret = bmp390_read_register(BMP390_REG_INT_CTRL, &reg, 1);
    if (ret != ESP_OK)
        return ret;

    // Decode interrupt configuration bits
    int_config->drdy_en   = (reg & (1 << 6)) ? true : false;
    int_config->fwtm_en   = (reg & (1 << 3)) ? true : false;
    int_config->ffull_en  = (reg & (1 << 4)) ? true : false;
    int_config->int_latch = (reg & (1 << 2)) ? true : false;
    int_config->int_od    = (reg & (1 << 0)) ? true : false;
    int_config->int_level = (reg & (1 << 1)) ? true : false;

    return ESP_OK;
}

/**
 * @brief Configure interrupt settings in INT_CTRL register
 * 
 * Sets up the interrupt behavior according to the provided configuration:
 * - Data Ready interrupt (new pressure/temperature data available)
 * - FIFO interrupts (watermark and full conditions)
 * - Interrupt pin characteristics (open-drain/push-pull, active high/low)
 * - Interrupt latching behavior
 *
 * @param[in] int_config Pointer to desired interrupt configuration
 * @return ESP_OK if successful, error code otherwise
 */
esp_err_t bmp390_set_int_config(const bmp390_int_config_t *int_config) {
    uint8_t config = 0;
    
    // Assemble configuration byte from settings
    if (int_config->drdy_en)   config |= (1 << 6);
    if (int_config->fwtm_en)   config |= (1 << 3);
    if (int_config->ffull_en)  config |= (1 << 4);
    if (int_config->int_latch) config |= (1 << 2);
    if (int_config->int_od)    config |= (1 << 0);
    if (int_config->int_level) config |= (1 << 1);

    return bmp390_write_register(BMP390_REG_INT_CTRL, &config, 1);
}