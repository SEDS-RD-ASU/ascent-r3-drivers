
// Several register mappings are courtesy of STMicroelectronics. See the below license.
// https://github.com/STMicroelectronics/lsm6dsv320x-pid/
//
// Copyright (c) 2019, STMicroelectronics
// All rights reserved.
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVkaf;jlsdENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
// OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


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


// REGISTER ADDRESSES -------------------------------------------------------------------------------
#define LSM6DSV320X_WHO_AM_I 0x0F
#define LSM6DSV320X_CTRL1 0x10
#define LSM6DSV320X_CTRL2 0x11
#define LSM6DSV320X_CTRL3 0x12
#define LSM6DSV320X_CTRL8 0x17


// FOR EVERYTHING BUT HIGH-G ACCELEROMETER! -------------------------------------------------------------------------------
typedef enum
{
  LSM6DSV320X_ODR_OFF              = 0x0,
  LSM6DSV320X_ODR_AT_1Hz875        = 0x1,
  LSM6DSV320X_ODR_AT_7Hz5          = 0x2,
  LSM6DSV320X_ODR_AT_15Hz          = 0x3,
  LSM6DSV320X_ODR_AT_30Hz          = 0x4,
  LSM6DSV320X_ODR_AT_60Hz          = 0x5,
  LSM6DSV320X_ODR_AT_120Hz         = 0x6,
  LSM6DSV320X_ODR_AT_240Hz         = 0x7,
  LSM6DSV320X_ODR_AT_480Hz         = 0x8,
  LSM6DSV320X_ODR_AT_960Hz         = 0x9,
  LSM6DSV320X_ODR_AT_1920Hz        = 0xA,
  LSM6DSV320X_ODR_AT_3840Hz        = 0xB,
  LSM6DSV320X_ODR_AT_7680Hz        = 0xC,
  LSM6DSV320X_ODR_HA01_AT_15Hz625  = 0x13,
  LSM6DSV320X_ODR_HA01_AT_31Hz25   = 0x14,
  LSM6DSV320X_ODR_HA01_AT_62Hz5    = 0x15,
  LSM6DSV320X_ODR_HA01_AT_125Hz    = 0x16,
  LSM6DSV320X_ODR_HA01_AT_250Hz    = 0x17,
  LSM6DSV320X_ODR_HA01_AT_500Hz    = 0x18,
  LSM6DSV320X_ODR_HA01_AT_1000Hz   = 0x19,
  LSM6DSV320X_ODR_HA01_AT_2000Hz   = 0x1A,
  LSM6DSV320X_ODR_HA01_AT_4000Hz   = 0x1B,
  LSM6DSV320X_ODR_HA01_AT_8000Hz   = 0x1C,
  LSM6DSV320X_ODR_HA02_AT_12Hz5    = 0x23,
  LSM6DSV320X_ODR_HA02_AT_25Hz     = 0x24,
  LSM6DSV320X_ODR_HA02_AT_50Hz     = 0x25,
  LSM6DSV320X_ODR_HA02_AT_100Hz    = 0x26,
  LSM6DSV320X_ODR_HA02_AT_200Hz    = 0x27,
  LSM6DSV320X_ODR_HA02_AT_400Hz    = 0x28,
  LSM6DSV320X_ODR_HA02_AT_800Hz    = 0x29,
  LSM6DSV320X_ODR_HA02_AT_1600Hz   = 0x2A,
  LSM6DSV320X_ODR_HA02_AT_3200Hz   = 0x2B,
  LSM6DSV320X_ODR_HA02_AT_6400Hz   = 0x2C,
  LSM6DSV320X_ODR_HA03_AT_13Hz     = 0x33,
  LSM6DSV320X_ODR_HA03_AT_26Hz     = 0x34,
  LSM6DSV320X_ODR_HA03_AT_52Hz     = 0x35,
  LSM6DSV320X_ODR_HA03_AT_104Hz    = 0x36,
  LSM6DSV320X_ODR_HA03_AT_208Hz    = 0x37,
  LSM6DSV320X_ODR_HA03_AT_417Hz    = 0x38,
  LSM6DSV320X_ODR_HA03_AT_833Hz    = 0x39,
  LSM6DSV320X_ODR_HA03_AT_1667Hz   = 0x3A,
  LSM6DSV320X_ODR_HA03_AT_3333Hz   = 0x3B,
  LSM6DSV320X_ODR_HA03_AT_6667Hz   = 0x3C,
} lsm6dsv320x_data_rate_t;

// ONLY FOR HIGH G ACCELEROMETER -------------------------------------------------------------------------------
typedef enum
{
  LSM6DSV320X_HG_XL_ODR_OFF         = 0x0,
  LSM6DSV320X_HG_XL_ODR_AT_480Hz    = 0x3,
  LSM6DSV320X_HG_XL_ODR_AT_960Hz    = 0x4,
  LSM6DSV320X_HG_XL_ODR_AT_1920Hz   = 0x5,
  LSM6DSV320X_HG_XL_ODR_AT_3840Hz   = 0x6,
  LSM6DSV320X_HG_XL_ODR_AT_7680Hz   = 0x7,
} lsm6dsv320x_hg_xl_data_rate_t;

// MODES -------------------------------------------------------------------------------
typedef enum
{
  LSM6DSV320X_XL_HIGH_PERFORMANCE_MD   = 0x0,
  LSM6DSV320X_XL_HIGH_ACCURACY_ODR_MD  = 0x1,
  LSM6DSV320X_XL_ODR_TRIGGERED_MD      = 0x3,
  LSM6DSV320X_XL_LOW_POWER_2_AVG_MD    = 0x4,
  LSM6DSV320X_XL_LOW_POWER_4_AVG_MD    = 0x5,
  LSM6DSV320X_XL_LOW_POWER_8_AVG_MD    = 0x6,
  LSM6DSV320X_XL_NORMAL_MD             = 0x7,
} lsm6dsv320x_xl_mode_t;

typedef enum
{
  LSM6DSV320X_GY_HIGH_PERFORMANCE_MD   = 0x0,
  LSM6DSV320X_GY_HIGH_ACCURACY_ODR_MD  = 0x1,
  LSM6DSV320X_GY_ODR_TRIGGERED_MD      = 0x3,
  LSM6DSV320X_GY_SLEEP_MD              = 0x4,
  LSM6DSV320X_GY_LOW_POWER_MD          = 0x5,
} lsm6dsv320x_gy_mode_t;

// SCALING -------------------------------------------------------------------------------
typedef enum
{
  LSM6DSV320X_250dps  = 0x1,
  LSM6DSV320X_500dps  = 0x2,
  LSM6DSV320X_1000dps = 0x3,
  LSM6DSV320X_2000dps = 0x4,
  LSM6DSV320X_4000dps = 0x5,
} lsm6dsv320x_gy_full_scale_t;

typedef enum
{
  LSM6DSV320X_2g  = 0x0,
  LSM6DSV320X_4g  = 0x1,
  LSM6DSV320X_8g  = 0x2,
  LSM6DSV320X_16g = 0x3,
} lsm6dsv320x_xl_full_scale_t;

typedef enum
{
  LSM6DSV320X_32g  = 0x0,
  LSM6DSV320X_64g  = 0x1,
  LSM6DSV320X_128g  = 0x2,
  LSM6DSV320X_256g = 0x3,
  LSM6DSV320X_320g = 0x4,
} lsm6dsv320x_hg_xl_full_scale_t;

// DRIVER FUNCTIONS -------------------------------------------------------------------------------

esp_err_t lsm_init(spi_host_device_t host);

esp_err_t lsm_set_lowgacc_odr(lsm6dsv320x_data_rate_t odr);
esp_err_t lsm_set_lowgacc_mode(lsm6dsv320x_xl_mode_t mode);
esp_err_t lsm_set_lowgacc_scale(lsm6dsv320x_xl_full_scale_t scale);

#endif