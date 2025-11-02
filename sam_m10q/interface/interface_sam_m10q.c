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

#include "ascent_r2_hardware_definition.h"
#include "i2c_manager.h"
#include "driver_SAM_M10Q.h"

#include "interface_sam_m10q.h"

#define GPS_RETRY_DELAY 0

// #define GPS_INIT_DEBUG

esp_err_t GPS_init(void) {
    esp_err_t ret;
    int fail = 0; // number of failed items

    sam_m10q_msginfo_t msginfo;
    uint8_t gps_packet_buf[100]; // max buffer size needed for initialization. ubx messages can of course be larger than 100 bytes.
    uint16_t gps_packet_length; 

    disableNMEAMessages();
    int attempts = 0;
    do {
        ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length);
        if (ret != ESP_OK) {
            vTaskDelay(GPS_RETRY_DELAY/portTICK_PERIOD_MS);
            attempts++;
        }

        #ifdef GPS_INIT_DEBUG
        if (msginfo.id != 0x01) {
            printf("Failed to disable NMEA messages, Retry # %d\n", attempts);
            for (int i = 0; i < gps_packet_length; i++) {
                printf("0x%02X ", gps_packet_buf[i]);
            }
            printf("\n");
        }
        #endif
   
    } while (
        ret != ESP_OK && 
        attempts < 100 && 
        msginfo.id != 0x01 // UBX-ACK-ACK
    );

    if (ret != ESP_OK) {
        fail ++;
    }

    setGPS10hz();
    attempts = 0;
    do {
        ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length);
        if (ret != ESP_OK) {
            vTaskDelay(GPS_RETRY_DELAY/portTICK_PERIOD_MS);
            attempts++;
        }
        #ifdef GPS_INIT_DEBUG
        if (msginfo.id != 0x01) {
            printf("Failed to set GPS to 10hz, Retry # %d\n", attempts);
            for (int i = 0; i < gps_packet_length; i++) {
                printf("0x%02X ", gps_packet_buf[i]);
            }
            printf("\n");
        }
        #endif
    } while (
        ret != ESP_OK &&
        attempts < 15 &&
        msginfo.id != 0x01 // UBX-ACK-ACK
    );

    if (ret != ESP_OK) {
        fail ++;
    }

    enableAllConstellations();
    attempts = 0;
    do {
        ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length);
        if (ret != ESP_OK) {
            vTaskDelay(GPS_RETRY_DELAY/portTICK_PERIOD_MS);
            attempts++;
        }
        #ifdef GPS_INIT_DEBUG
        if (msginfo.id != 0x01) {
            printf("Failed to enable all constellations, Retry # %d\n", attempts);
            for (int i = 0; i < gps_packet_length; i++) {
                printf("0x%02X ", gps_packet_buf[i]);
            }
            printf("\n");
        }
        #endif
    } while (
        ret != ESP_OK &&
        attempts < 15 &&
        msginfo.id != 0x01 // UBX-ACK-ACK
    );

    if (ret != ESP_OK) {
        fail ++;
    }

    if (fail > 0) { // if any of the initialization steps failed, return failure
        ret = ESP_FAIL;
    }

    return ret;
}


void GPS_read(GPS_data_t *gps_data) {
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
    };

    do {
        ret = readNextGPSPacket(&msginfo, gps_packet_buf, &gps_packet_length); // read the response (i.e. next packet from the GPS)
        if (ret != ESP_OK) {
            vTaskDelay(GPS_RETRY_DELAY/portTICK_PERIOD_MS);  // arbritary retry delay
            printf("GPS RETRY # %d\n", attempts + 1);
            attempts++;
        }
    } while (
        ret != ESP_OK &&
        attempts < 2 &&    // arbritary number
        msginfo.id != 0x07  // nav-pvt message ID
    );
    
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
}
