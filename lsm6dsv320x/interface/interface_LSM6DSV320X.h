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

#endif