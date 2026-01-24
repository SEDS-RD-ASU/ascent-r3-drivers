#ifndef DRIVER_LSM6DSV320X_H
#define DRIVER_LSM6DSV320X_H

#include "esp_err.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "esp_err.h"
#include "esp_log.h"
#include "spi_manager.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "ascent_r3_hardware_definition.h"

#define LSM6DSV320X_CTRL3 0x12

esp_err_t lsm_init(spi_host_device_t host);


#endif