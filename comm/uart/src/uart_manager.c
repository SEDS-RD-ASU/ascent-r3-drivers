#include "uart_manager.h"
#include <string.h>

static const char *TAG = "UART MANAGER";

// Static array to track the initialization of UART ports
static bool uart_initialized[3] = {false};

esp_err_t uart_manager_init(uart_port_t port, int rx, int tx, uint32_t baud, uart_parity_t parity, uart_stop_bits_t stop_bits, uart_hw_flowcontrol_t flow_control, uart_mode_t mode) 
{
    esp_err_t ret;

    uart_config_t uart_config = {
        .baud_rate = baud,
        .data_bits = UART_DATA_8_BITS,
        .parity = parity,
        .stop_bits = stop_bits,
        .flow_ctrl = flow_control,
        .rx_flow_ctrl_thresh = 122,
    };

    ret = uart_param_config(port, &uart_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set uart port parameters!");
        return ret;
    }

    ret = uart_set_pin(port, tx, rx, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set uart communication pins!");
        return ret;
    }

    // Install UART driver (TX buffer = 0, RX buffer = 1024, no queue, no interrupt flags)
    ret = uart_driver_install(port, 1024, 0, 0, NULL, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to install uart driver!");
        return ret;
    }

    uart_initialized[port] = true;

    return ESP_OK;
};

esp_err_t uart_flight_init(void)
{   
    esp_err_t ret;

    ret = uart_manager_init(UART_NUM_1,RF_UART_RX,RF_UART_TX,115200,UART_PARITY_DISABLE,UART_STOP_BITS_1,UART_HW_FLOWCTRL_DISABLE, UART_MODE_UART);

    const char *test = "Hello World!\n";

    uart_write_bytes(UART_NUM_1, test, strlen(test));

    if (ret != ESP_OK){
        ESP_LOGE(TAG, "Failed to initialize UART manager!");
        return ret;
    }

    ESP_LOGI(TAG, "SUCCESSFULLY INITIALIZED ALL UART BUSSES");

    return ret;
}

esp_err_t uart1_transmit(const uint8_t *data, size_t len)
{
    if (!uart_initialized[UART_NUM_1]) {
        ESP_LOGE(TAG, "UART1 not initialized!");
        return ESP_ERR_INVALID_STATE;
    }

    int bytes_written = uart_write_bytes(UART_NUM_1, (const char *)data, len);
    if (bytes_written < 0) {
        ESP_LOGE(TAG, "Failed to write to UART1!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

int uart1_receive(uint8_t *data, size_t max_len, uint32_t timeout_ms)
{
    if (!uart_initialized[UART_NUM_1]) {
        ESP_LOGE(TAG, "UART1 not initialized!");
        return -1;
    }

    return uart_read_bytes(UART_NUM_1, data, max_len, pdMS_TO_TICKS(timeout_ms));
}