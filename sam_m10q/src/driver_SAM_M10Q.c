#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_chip_info.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include <inttypes.h>
#include <string.h>
#include "esp_timer.h"

#include "driver_SAM_M10Q.h"

#include "ascent_r2_hardware_definition.h"
#include "i2c_manager.h"

#include <math.h>

// #define GPS_DEBUG

static uint8_t gps_packet_buf[GPS_MAX_PACKET_SIZE];


// ----------- GPS SPECIFIC I2C ------------------ //

static esp_err_t ubx_read_len(uint16_t *len) {
    uint8_t buf[2];
    uint8_t reg = 0xFD;

    // Write register address
    esp_err_t err = i2c_master_write_read_device(
        I2C_MASTER_PORT, SAM_M10Q_I2C_ADDR, &reg, 1, buf, 2, pdMS_TO_TICKS(100));
    if (err != ESP_OK) return err;

    *len = ((uint16_t)buf[0] << 8) | buf[1];
    return ESP_OK;
}

static esp_err_t ubx_read_data(uint8_t *data, size_t len) {
    uint8_t reg = 0xFF;
    return i2c_master_write_read_device(
        I2C_MASTER_PORT, SAM_M10Q_I2C_ADDR, &reg, 1, data, len, pdMS_TO_TICKS(100));
}

static esp_err_t read_gps_stream(uint8_t *data, uint16_t buf_length, uint16_t *real_length) {
    ubx_read_len(real_length); // read length of next UBX packet from 0xFD and 0xFE registers (see pg 23 of integration manual)
    if(*real_length == 0) { // if the length is zero there is nothing / something has gone horribly wrong
        #ifdef GPS_DEBUG
        printf("Nothing to read!!\n");
        #endif
        return ESP_FAIL;
    }
    if(*real_length > buf_length) { // not enough buffer to read message
        #ifdef GPS_DEBUG
        printf("ya fucked up. buf: %d, real: %d.\n", buf_length, *real_length);
        #endif
        return ESP_FAIL;
    }
    ubx_read_data(data,*real_length); // read the packet by repeatedly reading from 0xFF
    return ESP_OK;
}

// ------ actual driver follows ----  //

// This is the third full overhaul of this driver. I am really bad at writing drivers. This GPS is a pain in my ass. - Abdul

esp_err_t sendGPSBytes(uint8_t *buf, uint16_t num_bytes) {
    return i2c_manager_write_yeet(I2C_MASTER_PORT, SAM_M10Q_I2C_ADDR, buf, num_bytes);
}


esp_err_t readGPSBytes(uint8_t *buf, uint16_t num_bytes) {
    return i2c_manager_read_yeet(I2C_MASTER_PORT, SAM_M10Q_I2C_ADDR, buf, num_bytes);
}


esp_err_t readNextGPSPacket(sam_m10q_msginfo_t *msginfo, uint8_t *buf, uint16_t *buf_length) {
    // the goal of this function is to read the GPS stream successfully and return the UBX message for further processing

    uint16_t packet_length = 0;

    esp_err_t ret = read_gps_stream(gps_packet_buf, GPS_MAX_PACKET_SIZE, &packet_length);
    if(ret == ESP_FAIL){
        #ifdef GPS_DEBUG
        printf("Failed to read the GPS stream!\n");
        #endif
        return ESP_FAIL;
    }

    if (gps_packet_buf[0] != 0xB5 && gps_packet_buf[1] != 0x62) {
        #ifdef GPS_DEBUG
        printf("Invalid UBX packet\n");
        #endif
        return ESP_FAIL;
    } // check for validity (sync characters must be 0xb5 and 0x62)

    *msginfo = gpsIdentifyMessage(gps_packet_buf, packet_length); // use the header to identify the message type
    *buf_length = packet_length;

    #ifdef GPS_DEBUG
    printf("msginfo: class: 0x%02X, id: 0x%02X, length: %u\n", msginfo->class, msginfo->id, msginfo->length);

    printf("packet length: %d\n", packet_length);

    for(int i = 0; i < packet_length; i++){
        printf("0x%02X ", gps_packet_buf[i]);
    }

    printf("\n\n");
    #endif

    return ESP_OK;
}

esp_err_t disableNMEAMessages(void) {
    uint8_t disable_nmea_msg[] = {
        0xB5, 0x62, 0x06, 0x8A, 0x09, 0x00, 0x00, 0x01, 0x00, 0x00,
        0x02, 0x00, 0x72, 0x10, 0x00, 0x1E, 0xB1
    };
    return sendGPSBytes(disable_nmea_msg, sizeof(disable_nmea_msg));
}

esp_err_t setGPS10hz(void)
{
    uint8_t set_10hz_msg[] = {
        0XB5, 0X62, 0X6, 0X8A, 0XA, 0X0, 0X0, 0X1, 0X0, 0X0, 0X1, 0X0, 0X21, 0X30, 0X64, 0X0, 0X51, 0XB9
    };
    return sendGPSBytes(set_10hz_msg, sizeof(set_10hz_msg));
}

esp_err_t reqNAVPVT(void) {
    uint8_t req_navpvt_msg[] = {
        0xB5, 0x62, 0x01, 0x07, 0x00, 0x00, 0x08, 0x19
    };
    return sendGPSBytes(req_navpvt_msg, sizeof(req_navpvt_msg));
}

esp_err_t setAirborneDynamicModel(void) { // Airborne with <4g acceleration
    uint8_t set_airborne_dynamic_model_msg[] = {
        0XB5, 0X62, 0X6, 0X8A, 0X9, 0X0, 0X0, 0X1, 0X0, 0X0, 0X21, 0X0, 0X11, 0X20, 0X8, 0XF4, 0X51
    };
    return sendGPSBytes(set_airborne_dynamic_model_msg, sizeof(set_airborne_dynamic_model_msg));
}

esp_err_t enableAllConstellations(void) {
    uint8_t enable_all_constellations_msg[] = {
        0XB5, 0X62, 0X6, 0X8A, 0X4A, 0X0, 0X0, 0X1, 0X0, 0X0, 0X1F, 0X0, 0X31, 0X10, 0X1, 0X1, 0X0, 0X31, 0X10, 0X1, 0X20, 0X0, 0X31, 0X10, 0X1, 0X5, 0X0, 0X31, 0X10, 0X1, 0X21, 0X0, 0X31, 0X10, 0X1, 0X7, 0X0, 0X31, 0X10, 0X1, 0X22, 0X0, 0X31, 0X10, 0X1, 0XD, 0X0, 0X31, 0X10, 0X1, 0XF, 0X0, 0X31, 0X10, 0X1, 0X24, 0X0, 0X31, 0X10, 0X1, 0X12, 0X0, 0X31, 0X10, 0X1, 0X14, 0X0, 0X31, 0X10, 0X1, 0X25, 0X0, 0X31, 0X10, 0X1, 0X18, 0X0, 0X31, 0X10, 0X1, 0XA9, 0X93
    };
    return sendGPSBytes(enable_all_constellations_msg, sizeof(enable_all_constellations_msg));
}

sam_m10q_msginfo_t gpsIdentifyMessage(uint8_t *buf, uint16_t bufsize) {
    sam_m10q_msginfo_t msginfo;
    msginfo.class = buf[2];
    msginfo.id = buf[3];
    msginfo.length = (buf[5] << 8) | (buf[4]);
    msginfo.valid_checksum = false;

    // todo: Validate checksum

    uint8_t ck_a;
    uint8_t ck_b;

    ck_a = buf[bufsize - 2];
    ck_b = buf[bufsize - 1];

    return msginfo;
}

sam_m10q_navpvt_t gpsParseNavPVT() {
    // pov: parsing hell

    uint8_t payload[92] = {0};
    
    for (int i = 0; i < 92; i++) {
        payload[i] = gps_packet_buf[6 + i];
    } // this is slow. i know it is slow. - abdul

    sam_m10q_navpvt_t navpvt = {0};

    navpvt.iTOW = payload[0] | (payload[1] << 8) | (payload[2] << 16) | (payload[3] << 24); // ubx forums say no to use this....?
    navpvt.year = payload[4] | (payload[5] << 8);
    navpvt.month = payload[6];
    navpvt.day = payload[7];
    navpvt.hour = payload[8];
    navpvt.min = payload[9];
    navpvt.sec = payload[10];
    navpvt.valid = payload[11];
    navpvt.tAcc = payload[12] | (payload[13] << 8) | (payload[14] << 16) | (payload[15] << 24);
    navpvt.nano = payload[16] | (payload[17] << 8) | (payload[18] << 16) | (payload[19] << 24);

    navpvt.fixType = payload[20];
    navpvt.flags = payload[21];
    navpvt.flags2 = payload[22];
    navpvt.numSV = payload[23];
    navpvt.lon = payload[24] | (payload[25] << 8) | (payload[26] << 16) | (payload[27] << 24);
    navpvt.lat = payload[28] | (payload[29] << 8) | (payload[30] << 16) | (payload[31] << 24);
    navpvt.height = payload[32] | (payload[33] << 8) | (payload[34] << 16) | (payload[35] << 24);
    navpvt.hMSL = payload[36] | (payload[37] << 8) | (payload[38] << 16) | (payload[39] << 24);
    navpvt.hAcc = payload[40] | (payload[41] << 8) | (payload[42] << 16) | (payload[43] << 24);
    navpvt.vAcc = payload[44] | (payload[45] << 8) | (payload[46] << 16) | (payload[47] << 24);
    
    navpvt.velN = payload[48] | (payload[49] << 8) | (payload[50] << 16) | (payload[51] << 24);
    navpvt.velE = payload[52] | (payload[53] << 8) | (payload[54] << 16) | (payload[55] << 24);
    navpvt.velD = payload[56] | (payload[57] << 8) | (payload[58] << 16) | (payload[59] << 24);
    navpvt.gSpeed = payload[60] | (payload[61] << 8) | (payload[62] << 16) | (payload[63] << 24);
    navpvt.headMot = payload[64] | (payload[65] << 8) | (payload[66] << 16) | (payload[67] << 24);
    navpvt.sAcc = payload[68] | (payload[69] << 8) | (payload[70] << 16) | (payload[71] << 24);
    navpvt.headAcc = payload[72] | (payload[73] << 8) | (payload[74] << 16) | (payload[75] << 24);

    navpvt.pDOP = payload[76] | (payload[77] << 8);
    
    navpvt.flags3 = payload[78];

    // thank you for coming to my ted talk

    return navpvt;
}