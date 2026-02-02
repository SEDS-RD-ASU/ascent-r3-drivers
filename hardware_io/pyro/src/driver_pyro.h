#ifndef DRIVER_PYRO_H
#define DRIVER_PYRO_H

#include <stdbool.h>
#include "driver/gpio.h"
#include "adc_oneshot.h"
#include "esp_err.h"

// Pyro channel enumeration
typedef enum {
    PYRO_CHANNEL_1 = 1,    // Apogee
    PYRO_CHANNEL_2 = 2,    // Mains
    PYRO_CHANNEL_3 = 3,
    PYRO_CHANNEL_4 = 4
} pyro_channel_list_t;

typedef uint8_t pyro_channel_t;

typedef struct {
    uint64_t start_us;
    uint8_t duration_ms;
    bool state;

}pyro_state_t;

// Function prototypes
esp_err_t pyro_init(void);
bool pyro_continuity(pyro_channel_t channel);
esp_err_t pyro_activate(pyro_channel_t channel, uint8_t delay, bool bypass);
esp_err_t pyro_deactivate(pyro_channel_t channel);
esp_err_t pyro_update_state(void);
esp_err_t pyro_poll_state(pyro_state_t *state[5]);
void pyro_deinit(void);

#endif /* DRIVER_PYRO_H */
