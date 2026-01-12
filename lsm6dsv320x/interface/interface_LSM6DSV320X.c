#include "interface_LSM6DSV320X.h"

esp_err_t lsm_flight_init(spi_host_device_t host)
{
    return lsm_init(host);
}