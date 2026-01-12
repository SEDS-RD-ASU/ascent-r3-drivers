#ifndef DRIVER_LSM6DSV320X_H
#define DRIVER_LSM6DSV320X_H

#include "esp_err.h"
#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"
#include "esp_log.h"
#include "spi_manager.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

esp_err_t lsm_init(spi_host_device_t host);


#endif