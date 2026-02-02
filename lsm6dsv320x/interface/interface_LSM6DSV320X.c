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

    lsm_raw_data_t fakeRaw;
    lsm_raw_data_t fakeCalibrated;
    while(1)
    {
        lsm_get_raw(&fakeRaw);
        fakeCalibrated = fakeRaw;
        lsm_data_transform(&fakeCalibrated, &fakeRaw);
        // lsm_data_scale(&fake);
        // printf("Xg: %f, Yg: %f, Zg: %f\n", fakeRaw.gyr_x, fakeRaw.gyr_y, fakeRaw.gyr_z);
        // printf("Xla: %f, Yla: %f, Zla: %f\n", fakeCalibrated.lowacc_x, fakeCalibrated.lowacc_y, fakeCalibrated.lowacc_z);
        // printf("Xha: %f, Yha: %f, Zha: %f\n", fakeRaw.highacc_x, fakeRaw.highacc_y, fakeRaw.highacc_z);
        printf("Xha: %f, Yha: %f, Zha: %f\n", fakeCalibrated.highacc_x, fakeCalibrated.highacc_y, fakeCalibrated.highacc_z);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

esp_err_t lsm_data_transform(lsm_raw_data_t* dataOutput, lsm_raw_data_t* dataInput){
// Apply rotation matrix to gyro
#define INV_SQRT2 0.7071067811865475f

    if (dataOutput == NULL || dataInput == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    // // Gyro
    // dataOutput->gyr_x =
    //     (dataInput->gyr_x - dataInput->gyr_z) * INV_SQRT2;

    // dataOutput->gyr_y =
    //     dataInput->gyr_y; 

    // dataOutput->gyr_z =
    //     (dataInput->gyr_x + dataInput->gyr_z) * INV_SQRT2;

    // ---- Low-g accel ----
    // dataOutput->lowacc_x = dataInput->lowacc_x;

    // dataOutput->lowacc_y = ((dataInput->lowacc_y * 0.061f)/1000.0f) * 0.7071067811865475f;  

    // dataOutput->lowacc_z = dataInput->lowacc_z;

    // // High-G Accel
    dataOutput->highacc_x =
        (dataInput->highacc_x - dataInput->highacc_z) * INV_SQRT2;

    dataOutput->highacc_y =
        dataInput->highacc_y* INV_SQRT2;  
    dataOutput->highacc_z =
        (dataInput->highacc_x + dataInput->highacc_z) * INV_SQRT2;

    return ESP_OK;
}

esp_err_t lsm_data_scale(lsm_raw_data_t* data, lsm6dsv320x_gy_full_scale_t gyScale, 
    lsm6dsv320x_xl_full_scale_t xlScale, lsm6dsv320x_hg_xl_full_scale_t scale){
        data->gyr_x *= gyScale;
        data->gyr_y *= gyScale;
        data->gyr_z *= gyScale;
        data->lowacc_x *= xlScale;
        data->lowacc_y *= xlScale;
        data->lowacc_z *= xlScale;
    
        data->highacc_x *= scale;
        data->highacc_y *= scale;
        data->highacc_z *= scale;
    
        return ESP_OK;
    }

    
    