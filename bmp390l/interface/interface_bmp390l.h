#ifndef INTERFACE_BMP390L_H
#define INTERFACE_BMP390L_H

#include "driver_BMP390L.h"
#include "math.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "i2c_manager.h"
#include "ascent_r3_hardware_definition.h"

typedef struct {
    double pressure;
    double temperature;
    double alt;
} baro_double_t;

/**
 * @brief Store ground altitude for local reference frame
 * 
 * @param ground_alt Ground altitude in meters
 */
void bmp390_set_ground_alt(double ground_alt);

/**
 * @brief Set bias parameters for the BMP390 barometer
 * 
 * @param new_scaling Scaling factor for pressure measurements
 * @param new_bias Bias value for pressure measurements
 */
void bmp390_set_bias(float new_scaling, float new_bias);

/**
 * @brief Get altitude above ground level
 * 
 * @param baro_out Pointer to store altitude data in local reference frame
 */
void bmp390_get_local(baro_double_t* baro_out);

/**
 * @brief Fully initialize the BMP390 barometer for flight.
 * 
 * @param port I2C port being used for BMP390
 * @param int_cb Interrupt callback function
 * @param cb_args Arguments for the interrupt callback
 */
esp_err_t bmp390_flight_init(i2c_port_t port, void int_cb(void *args), void *cb_args);

/**
 * @brief Return the ground altitude used in the BMP390 driver.
 * 
 */
double bmp390_ground_altitude(void);

#endif /* INTERFACE_BMP390L_H */