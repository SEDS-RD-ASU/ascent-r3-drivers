#include "spi_manager.h"

static const char *TAG = "SPI MANAGER";

// Static array to track initialization status of SPI hosts
static bool spi_initialized[SPI_HOST_MAX] = {false};

esp_err_t spi_manager_init(spi_host_device_t host_id, int mosi_io_num, int miso_io_num, int sclk_io_num)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_io_num,
        .miso_io_num = miso_io_num,
        .sclk_io_num = sclk_io_num,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // .max_transfer_sz = 0 
        .max_transfer_sz = 8192
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(host_id, &bus_config, SPI_DMA_CH_AUTO);

    return ret;
}

esp_err_t spi_manager_initquad(spi_host_device_t host_id, int mosi_io_num, int miso_io_num, int sclk_io_num, int quadwp_io_num, int quadhd_io_num)
{
    spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_io_num,
        .miso_io_num = miso_io_num,
        .sclk_io_num = sclk_io_num,
        .quadwp_io_num = quadwp_io_num,
        .quadhd_io_num = quadhd_io_num,
        // .max_transfer_sz = 0 
        .max_transfer_sz = 8192
    };

    // Initialize SPI bus
    esp_err_t ret = spi_bus_initialize(host_id, &bus_config, SPI_DMA_CH_AUTO);

    return ret;
}

esp_err_t spi_manager_deinit(spi_host_device_t host_id)
{
    return spi_bus_free(host_id);
}

esp_err_t spi_flight_init(void)
{
    esp_err_t ret;

    ret = spi_manager_initquad(SPI2_HOST, R3_SPI2_MOSI, R3_SPI2_MISO, R3_SPI2_SCK, R3_SPI2_QUADWP, R3_SPI2_QUADHD);
    if(ret != ESP_OK) {ESP_LOGE(TAG, "FAILED TO INITIALIZE SPI2"); return ret;}
    ret = spi_manager_init(SPI3_HOST, R3_SPI3_MOSI, R3_SPI3_MISO, R3_SPI3_SCK);
    if(ret != ESP_OK) {ESP_LOGE(TAG, "FAILED TO INITIALIZE SPI3"); return ret;}

    ESP_LOGI(TAG, "SUCCESSFULLY INITIALIZED ALL SPI BUSSES");

    return ESP_OK;
}

esp_err_t spi_host_initialized(spi_host_device_t host)
{
    if (spi_initialized[host]) {
        return ESP_OK;
    }
    
    return ESP_FAIL;
}