#ifndef DRIVER_PSU_H
#define DRIVER_PSU_H

#include "esp_err.h"
#include "adc_oneshot.h"

// Battery voltage ranges
typedef struct {
    double min_v;
    double max_v;
} voltage_range_t;

#define BATTERY_VOLTAGE_RANGES { \
    {3.2, 4.2},   /* 1S Lithium */ \
    {6.4, 8.4},   /* 2S Lithium */ \
    {8.8, 9.6},   /* 9V */ \
    {9.6, 12.6},  /* 3S Lithium */ \
    {12.6, 13.0}, /* LiCB 12V */ \
    {1.8, 3.3},   /* 2S Alkaline */ \
}

typedef enum {
    POWER_SOURCE_BATTERY_ONLY,    // Only battery connected
    POWER_SOURCE_USB_ONLY,        // Only USB connected
    POWER_SOURCE_BOTH,            // Both USB and battery connected
    POWER_SOURCE_ERROR            // Error state
} power_source_t;

typedef enum {
    BATTERY_TYPE_1S_LITHIUM,
    BATTERY_TYPE_2S_LITHIUM,
    BATTERY_TYPE_9V,
    BATTERY_TYPE_3S_LITHIUM,
    BATTERY_TYPE_LICB_12V,
    BATTERY_TYPE_2S_ALKALINE,
    BATTERY_TYPE_UNKNOWN,
    BATTERY_TYPE_NONE  // Used when no battery is connected
} battery_type_t;

typedef struct {
    power_source_t source;
    battery_type_t battery_type;
    int battery_percentage;  // -1 if not applicable
} power_status_t;

// Function prototypes
esp_err_t psu_init(battery_type_t battery_type);
esp_err_t psu_init_default(void);
esp_err_t psu_init_with_adc(battery_type_t battery_type, adc_oneshot_unit_handle_t adc_handle);
esp_err_t psu_init_default_with_adc(adc_oneshot_unit_handle_t adc_handle);
double psu_read_battery_voltage(void);
power_status_t psu_get_power_source(void);
void psu_deinit(void);

// Battery-related functions
battery_type_t psu_autodetect_battery(void);
int psu_get_battery_percentage(void);
battery_type_t psu_get_battery_type(void);

// Add PSU mode enum
typedef enum {
    PSU_MODE_BUCK,           // Input voltage > 3.778V
    PSU_MODE_BOOST,          // Input voltage < 3.778V
    PSU_MODE_PERFECT_SOURCE, // Input voltage = 3.778V (±0.1V)
    PSU_MODE_ERROR          // Error state
} psu_mode_t;

// Add new function prototype
psu_mode_t psu_get_mode(void);

#endif /* DRIVER_PSU_H */
