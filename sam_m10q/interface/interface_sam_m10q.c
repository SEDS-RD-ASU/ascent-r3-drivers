#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_task_wdt.h"
#include <inttypes.h>

#include "driver/gpio.h"

#include "ascent_r3_hardware_definition.h"
#include "i2c_manager.h"
#include "driver_SAM_M10Q.h"

#include "interface_sam_m10q.h"

static const char *TAG = "SAM-M10Q INTERFACE";

#define GPS_RETRY_DELAY 1
#define MAX_ATTEMPTS 20

// #define GPS_INIT_DEBUG

esp_err_t GPS_init(i2c_port_t port) {
    esp_err_t ret;
    int fail = 0; // number of failed items

    if(!i2c_manager_is_initialized(port)) {
        ESP_LOGE(TAG, "I2C PORT NOT INITIALIZED!");
        return ESP_FAIL;
    }

    gpio_set_direction(SAM_M10Q_RESET,GPIO_MODE_OUTPUT);
    gpio_set_direction(SAM_M10Q_TIMEPULSE,GPIO_MODE_INPUT);

    gpio_set_level(SAM_M10Q_RESET,0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level(SAM_M10Q_RESET,1);

    vTaskDelay (pdMS_TO_TICKS(500));

    gps_set_i2c_port(port);

    sam_m10q_msginfo_t msginfo;
    uint8_t gps_packet_buf[100]; // max buffer size needed for initialization. ubx messages can of course be larger than 100 bytes.
    uint16_t gps_packet_length; 

    disableNMEAMessages();
    int attempts = 0;
    do {
        ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length);
        if (ret != ESP_OK) {
            #ifdef GPS_INIT_DEBUG
            printf("readnextgps packet failed w/ error code: %d\n\n", ret);
            #endif
            vTaskDelay(GPS_RETRY_DELAY/portTICK_PERIOD_MS);
            attempts++;
        }

        #ifdef GPS_INIT_DEBUG
        if (msginfo.id != 0x01) {
            printf("Failed to disable NMEA messages, Retry # %d\n", attempts);
        }
        #endif
   
    } while (
        ret != ESP_OK && 
        attempts < MAX_ATTEMPTS && 
        msginfo.id != 0x01 // UBX-ACK-ACK
    );

    if (ret != ESP_OK){
        fail++;
        #ifdef GPS_INIT_DEBUG
        printf("Failed to disable NMEA messages! Fail: %d\n\n", fail);
        #endif
    } else {
        #ifdef GPS_INIT_DEBUG
        printf("Successfully disabled NMEA messages! Fail: %d\n\n", fail);
        #endif
    }

    setGPS25hz();
    ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length);
    if (ret) return ret;
    if (msginfo.id == 0x01) {
        #ifdef GPS_INIT_DEBUG
        printf("Successfully set 25hz! Fail: %d\n\n", fail);
        #endif
    } else {
        printf("Failed to set 25hz!\n");
        fail++;
    }

    enableOnlyGPS();
    ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length);
    if (ret) return ret;
    if (msginfo.id == 0x01) {
        printf("Successfully enable only GPS! Fail: %d\n\n", fail);
    } else {
        printf("Failed to enable only GPS!\n");
        fail++;
    }

    enableTXReady();
    ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length);
    if (ret) return ret;
    if (msginfo.id == 0x01) {
        #ifdef GPS_INIT_DEBUG
        printf("Successfully set TXReady pin! Fail: %d\n\n", fail);
        #endif
    } else {
        printf("Failed to set TXReady pin!\n");
        fail++;
    }

    if (fail) { 
        return ESP_FAIL;
    }

    return ESP_OK;
}


esp_err_t GPS_read(GPS_data_t *gps_data)
{
    esp_err_t ret;
    sam_m10q_msginfo_t msginfo;
    uint8_t gps_packet_buf[GPS_MAX_PACKET_SIZE];
    uint16_t gps_packet_length;

    int attempts = 0;

    ret = reqNAVPVT(); // request NAV-PVT from the GPS
    if (ret != ESP_OK) {
        gps_data->UTCtstamp = 0;
        gps_data->lon = 0;
        gps_data->lat = 0;
        gps_data->hMSL = 0;
        gps_data->height = 0;
        gps_data->fixType = 0;
        gps_data->numSV = 0;
        printf("!!!!! WRITING TO GPS FAILED !!!!!!\n"); // todo: send the board into a fail state
        return ESP_FAIL;
    };

    do {
        ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length); // read the response (i.e. next packet from the GPS)
        if (ret != ESP_OK) {
            vTaskDelay(GPS_RETRY_DELAY/portTICK_PERIOD_MS);  // arbritary retry delay
            // printf("GPS RETRY # %d\n", attempts + 1);
            attempts++;
        }
    } while (
        ret != ESP_OK &&
        attempts < MAX_ATTEMPTS &&
        msginfo.id != 0x07  // nav-pvt message ID
    );

    if(attempts==MAX_ATTEMPTS){
        gps_data->UTCtstamp = 0;
        gps_data->lon = 0;
        gps_data->lat = 0;
        gps_data->hMSL = 0;
        gps_data->height = 0;
        gps_data->fixType = 0;
        gps_data->numSV = 0;
        
        ESP_LOGE(TAG, "EXCEEDED %d ATTEMPTS WHILE TRYING TO GET NAVPVT", attempts);
        return ESP_FAIL;
    }
    
    sam_m10q_navpvt_t navpvt = gpsParseNavPVT(); // now that we have a nav-pvt message, parse useful info from it

    // yeet the information at pointers
    // this is what the flight state logic and telemetry will use
    gps_data->UTCtstamp = navpvt.iTOW;
    gps_data->lon = navpvt.lon;
    gps_data->lat = navpvt.lat;
    gps_data->hMSL = navpvt.hMSL;
    gps_data->height = navpvt.height;
    gps_data->fixType = navpvt.fixType;
    gps_data->numSV = navpvt.numSV;

    return ESP_OK;
}
