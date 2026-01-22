#ifndef SPI_MANAGER_H
#define SPI_MANAGER_H

#include "driver/spi_common.h"
#include "esp_err.h"
#include "ascent_r3_hardware_definition.h"
#include "esp_log.h"

esp_err_t spi_manager_init(spi_host_device_t host_id, int mosi_io_num, int miso_io_num, int sclk_io_num);
esp_err_t spi_manager_initquad(spi_host_device_t host_id, int mosi_io_num, int miso_io_num, int sclk_io_num, int quadwp_io_num, int quadhd_io_num);
esp_err_t spi_manager_deinit(spi_host_device_t host_id);

esp_err_t spi_flight_init(void);

esp_err_t spi_host_initialized(spi_host_device_t host);

#endif