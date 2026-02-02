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
#include "math.h"

esp_err_t lsm_flight_init(spi_host_device_t host);

esp_err_t lsm_get_local(lsm_raw_data_t* local);

#endif