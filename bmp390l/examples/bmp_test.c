// #include <stdio.h>
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #include "esp_err.h"
// #include "driver_BMP390L.h"
// #include "ascent_r3_hardware_definition.h"
// #include "i2c_manager.h"

// static esp_err_t bmpValidate(void) {
//     printf("\n=== Current Sensor Configuration ===\n");
//     fflush(stdout);
//     esp_err_t ret = ESP_OK;

//     // 1. Chip ID and Revision ID
//     uint8_t chip_id, rev_id;
//     ret = bmp390_get_chip_id(&chip_id);
//     ret |= bmp390_get_rev_id(&rev_id);
//     printf("Chip ID: 0x%02X\n", chip_id);
//     printf("Revision ID: 0x%02X\n", rev_id);
//     fflush(stdout);
//     vTaskDelay(pdMS_TO_TICKS(100));

//     // 2. Power Control Settings
//     bmp390_pwr_ctrl_t pwr_ctrl;
//     ret |= bmp390_get_pwr_ctrl(&pwr_ctrl);
//     printf("Power Control:\n");
//     printf("  - Pressure Enabled: %s\n", pwr_ctrl.press_en ? "Yes" : "No");
//     printf("  - Temperature Enabled: %s\n", pwr_ctrl.temp_en ? "Yes" : "No");
//     printf("  - Mode: %d (0=Sleep, 1=Forced, 3=Normal)\n", pwr_ctrl.mode);

//     // 3. Oversampling Settings
//     bmp390_osr_settings_t osr;
//     ret |= bmp390_get_osr(&osr);
//     printf("Oversampling:\n");
//     printf("  - Pressure: %d (0=x1, 1=x2, 2=x4, 3=x8, 4=x16, 5=x32)\n", osr.press_os);
//     printf("  - Temperature: %d (0=x1, 1=x2, 2=x4, 3=x8, 4=x16, 5=x32)\n", osr.temp_os);

//     // 4. Output Data Rate
//     bmp390_odr_t odr;
//     ret |= bmp390_get_odr(&odr);
//     printf("Output Data Rate: %d\n", odr);
//     printf("  (0=200Hz, 1=100Hz, 2=50Hz, 3=25Hz, 4=12.5Hz, 5=6.25Hz, etc)\n");

//     // 5. Filter Configuration
//     bmp390_config_t config;
//     ret |= bmp390_get_config(&config);
//     printf("IIR Filter: %d\n", config.iir_filter);
//     printf("  (0=Bypass, 1=Coeff2, 2=Coeff4, 3=Coeff8, 4=Coeff16, etc)\n");

//     // 6. Interrupt Configuration
//     bmp390_int_config_t int_cfg;
//     ret |= bmp390_get_int_config(&int_cfg);
//     if (ret == ESP_OK) {
//         printf("\nInterrupt Configuration:\n");
//         printf("  - Data Ready Interrupt: %s\n", int_cfg.drdy_en ? "Enabled" : "Disabled");
//         printf("  - FIFO Watermark Interrupt: %s\n", int_cfg.fwtm_en ? "Enabled" : "Disabled");
//         printf("  - FIFO Full Interrupt: %s\n", int_cfg.ffull_en ? "Enabled" : "Disabled");
//         printf("  - Interrupt Latching: %s\n", int_cfg.int_latch ? "Enabled" : "Disabled");
//         printf("  - Output Type: %s\n", int_cfg.int_od ? "Open-Drain" : "Push-Pull");
//         printf("  - Active Level: %s\n", int_cfg.int_level ? "High" : "Low");
//     }

//     // 7. Error and Status Registers
//     uint8_t err_reg;
//     ret |= bmp390_get_err_reg(&err_reg);
//     printf("Error Register: 0x%02X\n", err_reg);

//     bmp390_status_t status;
//     ret |= bmp390_get_status(&status);
//     printf("Command Ready Status: %s\n\n", status.cmd_rdy ? "Ready" : "Busy");
//     fflush(stdout);

//     return ret;
// }

// static esp_err_t bmpRead(void) {
//     printf("=== Starting Test Readings ===\n\n");
//     fflush(stdout);
    
//     esp_err_t ret = ESP_OK;
//     double pressure, temperature;
    
//     for (int i = 0; i < 4; i++) {
//         ret = bmp390_read_sensor_data(&pressure, &temperature);
//         if (ret == ESP_OK) {
//             printf("Pressure: %.2f hPa, Temperature: %.2f °C\n", pressure, temperature);
//         } else {
//             printf("Failed to read sensor data (error: %d)\n", ret);
//         }
//         fflush(stdout);
//         vTaskDelay(pdMS_TO_TICKS(250));
//     }
    
//     return ret;
// }

// void BMPTest(void) {
//     vTaskDelay(pdMS_TO_TICKS(1000));
    
//     printf("\n\n=== BMP390L Hardware Test ===\n");
//     fflush(stdout);
//     vTaskDelay(pdMS_TO_TICKS(100));
    
//     // Initialize sensor
//     printf("Initializing BMP390L with SDA=%d, SCL=%d, Port=%d...\n", 
//            I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_PORT);
//     fflush(stdout);
    
//     esp_err_t ret = bmp390_init(I2C_MASTER_PORT);
//     if (ret != ESP_OK) {
//         printf("Failed to initialize BMP390L (error: %d)\n", ret);
//         fflush(stdout);
//         return;
//     }
//     printf("BMP390L initialization successful\n\n");
//     fflush(stdout);
//     vTaskDelay(pdMS_TO_TICKS(100));

//     // Validate sensor configuration
//     ret = bmpValidate();
//     if (ret != ESP_OK) {
//         printf("Sensor validation failed (error: %d)\n", ret);
//         return;
//     }

//     // Read sensor data
//     ret = bmpRead();
//     if (ret != ESP_OK) {
//         printf("Sensor reading test failed (error: %d)\n", ret);
//     }
// }