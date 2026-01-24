#include "interface_LSM6DSV320X.h"

esp_err_t lsm_flight_init(spi_host_device_t host)
{
    esp_err_t ret;

    ret = lsm_init(host);
    if(ret) return ret;

    ret = lsm_set_lowgacc_odr(LSM6DSV320X_ODR_AT_7680Hz);
    if(ret) return ret;

    ret = lsm_set_lowgacc_mode(LSM6DSV320X_XL_HIGH_ACCURACY_ODR_MD);
    if(ret) return ret;
    
    ret = lsm_set_lowgacc_scale(LSM6DSV320X_16g);
    if(ret) return ret;

    ret = lsm_set_highgacc_odr(LSM6DSV320X_HG_XL_ODR_AT_7680Hz);
    if(ret) return ret;

    ret = lsm_set_highgacc_scale(LSM6DSV320X_320g);
    if(ret) return ret;
    
    ret = lsm_set_gyr_odr(LSM6DSV320X_ODR_AT_7680Hz);
    if(ret) return ret;

    ret = lsm_set_gyr_mode(LSM6DSV320X_GY_HIGH_ACCURACY_ODR_MD);
    if(ret) return ret;

    ret = lsm_set_gyr_scale(LSM6DSV320X_4000dps);
    if(ret) return ret;

    return ESP_OK;
}