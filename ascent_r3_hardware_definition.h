#ifndef ASCENT_R3_PINS_H
#define ASCENT_R3_PINS_H

#include "driver/gpio.h"
#include "driver/i2c.h"

/* I2C Bus Configuration */

#define I2C_MASTER_FREQ_HZ          400000          // I2C master clock frequency

// I2C 0 (SAM-M10Q)
#define R3_SDA0                     GPIO_NUM_35
#define R3_SCL0                     GPIO_NUM_36
#define R3_I2C0_PORT                I2C_NUM_0

// I2C 1 (BMP390)
#define R3_SDA1                     GPIO_NUM_38
#define R3_SCL1                     GPIO_NUM_37
#define R3_I2C1_PORT                I2C_NUM_1

/* SPI Bus Configuration */

// SPI2 (IO_MUX) for W25Q512 flash. See https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/spi_master.html#gpio-matrix-and-io-mux
#define R3_SPI2_SCK                 GPIO_NUM_12
#define R3_SPI2_MOSI                GPIO_NUM_11
#define R3_SPI2_MISO                GPIO_NUM_13
#define R3_SPI2_QUADHD              GPIO_NUM_9
#define R3_SPI2_QUADWP              GPIO_NUM_14

// SPI3 (GPIO MATRIX) for LSM6DSV320XTR IMU.
#define R3_SPI3_SCK                 GPIO_NUM_18
#define R3_SPI3_MOSI                GPIO_NUM_21
#define R3_SPI3_MISO                GPIO_NUM_33

/* UART Configuration */

#define RF_UART_TX                  GPIO_NUM_43
#define RF_UART_RX                  GPIO_NUM_44

/* Sensor-Specific Pins */

// BMP390L Barometer
#define PIN_BMP390_INT              GPIO_NUM_1     // BMP390L interrupt pin
#define BMP390_I2C_ADDR             0x76            // BMP390L I2C address

// SAM-M10Q GPS
#define SAM_M10Q_I2C_ADDR           0x42            // SAM-M10Q I2C address
#define SAM_M10Q_TIMEPULSE          GPIO_NUM_15
#define SAM_M10Q_RESET              GPIO_NUM_8

// W25Q512 Flash
#define FLASH_CS                    GPIO_NUM_10     // W25Q512 Flash CS pin

// LSM6DSV320XTR IMU
#define IMU_INT1                    GPIO_NUM_34
#define IMU_INT2                    GPIO_NUM_16
#define IMU_CS                      GPIO_NUM_17

/* Power supply and pyro pins */

#define VBATT                       GPIO_NUM_7     // VBATT

// Pyrotechnic channel Pins
#define PYRO1_OUT                   GPIO_NUM_40     // Pyro channel 1 (APOGEE)
#define PYRO2_OUT                   GPIO_NUM_39     // Pyro channel 2 (MAINS)
#define PYRO3_OUT                   GPIO_NUM_41     // Pyro channel 3
#define PYRO4_OUT                   GPIO_NUM_42     // Pyro channel 4
#define PYRO1_CONT                  GPIO_NUM_5      // Pyro Continuity channel 1 (APOGEE)
#define PYRO2_CONT                  GPIO_NUM_6      // Pyro Continuity channel 2 (MAINS)
#define PYRO3_CONT                  GPIO_NUM_4      // Pyro Continuity channel 3
#define PYRO4_CONT                  GPIO_NUM_3      // Pyro Continuity channel 4

// todo: implement this into the driver properly...
#define DIODE_DROP 0.35f
#define DIVIDER_RATIO 3.778f
#define MAGIC 1.333

/* Indicator pins*/

#define PIN_LED                     GPIO_NUM_48
#define PIN_BUZZER                  GPIO_NUM_2

// todo: move this to driver or remove it since mutexing busses is not needed on R3
#define MUTEX_TIMEOUT 100

#endif /* ASCENT_R3_PINS_H */ 