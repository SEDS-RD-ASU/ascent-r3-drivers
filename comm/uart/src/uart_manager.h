#ifndef UART_MANAGER_H
#define UART_MANAGER_H

#include "stdbool.h"
#include "stdint.h"
#include "esp_log.h"
#include "esp_err.h"
#include "driver/uart.h"
#include "ascent_r3_hardware_definition.h"

esp_err_t uart_manager_init(uart_port_t port, int rx, int tx, uint32_t baud, uart_parity_t parity, uart_stop_bits_t stop_bits, uart_hw_flowcontrol_t flow_control, uart_mode_t mode);

esp_err_t uart_flight_init(void);

esp_err_t uart0_transmit(const uint8_t *data, size_t len);

#endif