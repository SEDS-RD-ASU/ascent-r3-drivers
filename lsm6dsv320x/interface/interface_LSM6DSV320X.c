#include "interface_LSM6DSV320X.h"

esp_err_t lsm_flight_init(spi_host_device_t host)
{
    esp_err_t ret;

    ret = lsm_init(host);
    if(ret) return ret;
    
    ret = lsm_set_lowgacc_mode(LSM6DSV320X_XL_HIGH_ACCURACY_ODR_MD);
    if(ret) return ret;

    ret = lsm_set_lowgacc_odr(LSM6DSV320X_ODR_AT_7680Hz);
    if(ret) return ret;

    ret = lsm_set_lowgacc_scale(LSM6DSV320X_16g);
    if(ret) return ret;

    ret = lsm_set_highgacc_odr(LSM6DSV320X_HG_XL_ODR_AT_7680Hz);
    if(ret) return ret;

    ret = lsm_set_highgacc_scale(LSM6DSV320X_320g);
    if(ret) return ret;

    ret = lsm_set_gyr_mode(LSM6DSV320X_GY_HIGH_ACCURACY_ODR_MD);
    if(ret) return ret;
    
    ret = lsm_set_gyr_odr(LSM6DSV320X_ODR_AT_7680Hz);
    if(ret) return ret;

    ret = lsm_set_gyr_scale(LSM6DSV320X_4000dps);
    if(ret) return ret;

    return ESP_OK;
}

// Transform sensor x and y data s.t. y axis is oriented along long side, x axis is oriented along short side.
// y
// ^
// |
// |
// |
// 0 - - - - - > x
static esp_err_t lsm_transform(lsm_raw_data_t* data){
    
    float transformed_gyr_x = (-0.707106f * data->gyr_x) - (0.707106f * data->gyr_y);
    float transformed_gyr_y = (0.707106f * data->gyr_x) - (0.707106f * data->gyr_y);

    float transformed_lowacc_x = (-0.707106f * data->lowacc_x) - (0.707106f * data->lowacc_y);
    float transformed_lowacc_y = -1*((0.707106f * data->lowacc_x) - (0.707106f * data->lowacc_y));

    float transformed_highacc_x = (-0.707106f * data->highacc_x) - (0.707106f * data->highacc_y);
    float transformed_highacc_y = -1*((0.707106f * data->highacc_x) - (0.707106f * data->highacc_y));

    data->gyr_x = transformed_gyr_x;
    data->gyr_y = transformed_gyr_y;

    data->lowacc_x = transformed_lowacc_x;
    data->lowacc_y = transformed_lowacc_y;

    data->highacc_x = transformed_highacc_x;
    data->highacc_y = transformed_highacc_y;

    return ESP_OK;
}

// For 16g/320g & 4000dps mode only. This is a temporary patch that needs to be revisited.
static esp_err_t lsm_scale(lsm_raw_data_t* data)
{
    float scaled_gyr_x = data->gyr_x * 0.14f;
    float scaled_gyr_y = data->gyr_y * 0.14f;
    float scaled_gyr_z = data->gyr_z * 0.14f;

    float scaled_lowacc_x = data->lowacc_x * 0.00478728f;
    float scaled_lowacc_y = data->lowacc_y * -0.00478728f;
    float scaled_lowacc_z = data->lowacc_z * 0.00478728f;
    
    float scaled_highacc_x = data->highacc_x * 0.10219077f;
    float scaled_highacc_y = data->highacc_y * -0.10219077f;
    float scaled_highacc_z = data->highacc_z * 0.10219077f;

    data->gyr_x = scaled_gyr_x;
    data->gyr_y = scaled_gyr_y;
    data->gyr_z = scaled_gyr_z;

    data->lowacc_x = scaled_lowacc_x;
    data->lowacc_y = scaled_lowacc_y;
    data->lowacc_z = scaled_lowacc_z;

    data->highacc_x = scaled_highacc_x;
    data->highacc_y = scaled_highacc_y;
    data->highacc_z = scaled_highacc_z;

    return ESP_OK;
}

esp_err_t lsm_get_local(lsm_raw_data_t *local)
{
    esp_err_t ret = lsm_get_raw(local);
    if(ret) return ret;

    lsm_transform(local);
    lsm_scale(local);

    return ESP_OK;
}
    