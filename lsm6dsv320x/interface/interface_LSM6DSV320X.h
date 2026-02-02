#ifndef INTERFACE_LSM6DSV320X_H
#define INTERFACE_LSM6DSV320X_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "spi_manager.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver_LSM6DSV320X.h"

esp_err_t lsm_flight_init(spi_host_device_t host);

esp_err_t lsm_data_transform(lsm_raw_data_t* dataOutput, lsm_raw_data_t* dataInput);
esp_err_t lsm_data_scale(lsm_raw_data_t* data, lsm6dsv320x_gy_full_scale_t gyScale, 
    lsm6dsv320x_xl_full_scale_t xlScale, lsm6dsv320x_hg_xl_full_scale_t scale);
esp_err_t lsm_get_data(lsm_raw_data_t* data);

#endif