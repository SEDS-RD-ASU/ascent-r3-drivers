#ifndef I2C_MANAGER_H
#define I2C_MANAGER_H

#include "driver/i2c.h"
#include "esp_err.h"
#include "ascent_r3_hardware_definition.h"
#include "esp_log.h"

/**
 * @brief Initialize the I2C driver with the specified configuration
 * 
 * @param sda_pin GPIO number for SDA
 * @param scl_pin GPIO number for SCL
 * @param freq_hz Clock frequency in Hz
 * @param port I2C port number (I2C_NUM_0 or I2C_NUM_1)
 * @return esp_err_t ESP_OK on success, appropriate error code otherwise
 */
esp_err_t i2c_manager_init(int sda_pin, int scl_pin, uint32_t freq_hz, i2c_port_t port);

/**
 * @brief Check if I2C is already initialized for the specified port
 * 
 * @param port I2C port number to check
 * @return true if initialized, false otherwise
 */
bool i2c_manager_is_initialized(i2c_port_t port);

/**
 * @brief Deinitialize the I2C driver for the specified port
 * 
 * @param port I2C port number to deinitialize
 * @return esp_err_t ESP_OK on success, appropriate error code otherwise
 */
esp_err_t i2c_manager_deinit(i2c_port_t port);

/**
 * @brief Read data from an I2C device register
 * 
 * @param port I2C port number
 * @param device_addr I2C device address
 * @param reg Register address to read from
 * @param data Pointer to buffer to store read data
 * @param len Number of bytes to read
 * @return esp_err_t ESP_OK on success, appropriate error code otherwise
 */
esp_err_t i2c_manager_read_register(i2c_port_t port, uint8_t device_addr, 
                                  uint8_t reg, uint8_t *data, size_t len);

/**
 * @brief Write data to an I2C device register
 * 
 * @param port I2C port number
 * @param device_addr I2C device address
 * @param reg Register address to write to
 * @param data Pointer to data to write
 * @param len Number of bytes to write
 * @return esp_err_t ESP_OK on success, appropriate error code otherwise
 */
esp_err_t i2c_manager_write_register(i2c_port_t port, uint8_t device_addr, 
                                   uint8_t reg, uint8_t *data, size_t len);

esp_err_t i2c_manager_write_yeet(i2c_port_t port, uint8_t device_addr, uint8_t *data, size_t len);
esp_err_t i2c_manager_read_yeet(i2c_port_t port, uint8_t device_addr, uint8_t *data, size_t len);

esp_err_t i2c_flight_init();

esp_err_t i2c_scan_t(i2c_port_t port);

#endif // I2C_MANAGER_H 
