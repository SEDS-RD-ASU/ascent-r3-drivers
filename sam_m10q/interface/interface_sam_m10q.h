#ifndef INTERFACE_SAM_M10Q_H
#define INTERFACE_SAM_M10Q_H

#include "driver_SAM_M10Q.h"

typedef struct {
    uint32_t UTCtstamp;
    int32_t lon;
    int32_t lat;
    int32_t height;
    int32_t hMSL;
    uint8_t fixType;
    uint8_t numSV;
} GPS_data_t;

void GPS_read(GPS_data_t *gps_data);

esp_err_t GPS_init(void);

#endif /* INTERFACE_SAM_M10Q_H */
