#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "driver_pyro.h"
#include "ascent_r3_hardware_definition.h"

static esp_err_t pyroValidate(void) {
    printf("\n=== Pyro Channels Status ===\n");
    fflush(stdout);
    esp_err_t ret = ESP_OK;

    // Test continuity and resistance for each channel
    for (pyro_channel_t channel = PYRO_CHANNEL_1; channel <= PYRO_CHANNEL_COUNT; channel++) {
        printf("\nChannel %d (", channel);
        switch(channel) {
            case PYRO_CHANNEL_1:
                printf("APOGEE");
                break;
            case PYRO_CHANNEL_2:
                printf("MAINS");
                break;
            case PYRO_CHANNEL_3:
                printf("AUX");
                break;
        }
        printf("):\n");

        // Check continuity
        bool cont = pyro_continuity(channel);
        printf("  - Continuity: %s\n", cont ? "DETECTED" : "NONE");
        
        fflush(stdout);
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return ret;
}

static esp_err_t pyroActivationTest(void) {
//     printf("\n=== Pyro Activation Test ===\n");
//     printf("WARNING: This will attempt to fire pyro channels!\n");
//     printf("Ensure proper safety measures are in place.\n\n");
//     fflush(stdout);
    
    esp_err_t ret = ESP_OK;
    
//     for (pyro_channel_t channel = PYRO_CHANNEL_1; channel <= PYRO_CHANNEL_COUNT; channel++) {
//         printf("Testing Channel %d activation... ", channel);
//         fflush(stdout);
        
//         ret = pyro_activate(channel);
//         if (ret == ESP_OK) {
//             printf("SUCCESS\n");
//         } else if (ret == ESP_ERR_INVALID_STATE) {
//             printf("FAILED (No continuity)\n");
//         } else {
//             printf("FAILED (Error: %d)\n", ret);
//         }
        
//         fflush(stdout);
//         vTaskDelay(pdMS_TO_TICKS(500)); // Half second delay between channels
//     }
    
    return ret;
}

void PyroTest(void) {
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    printf("\n\n=== Pyro Channels Hardware Test ===\n");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Initialize pyro channels
    printf("Initializing Pyro channels...\n");
    fflush(stdout);
    
    esp_err_t ret = pyro_init();
    if (ret != ESP_OK) {
        printf("Failed to initialize Pyro channels (error: %d)\n", ret);
        fflush(stdout);
        return;
    }
    printf("Pyro channels initialization successful\n\n");
    fflush(stdout);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Validate pyro channels
    ret = pyroValidate();
    if (ret != ESP_OK) {
        printf("Pyro channels validation failed (error: %d)\n", ret);
        return;
    }

    // Optional: Uncomment to test actual pyro activation
    // WARNING: This will attempt to fire the charges!
    /*
    ret = pyroActivationTest();
    if (ret != ESP_OK) {
        printf("Pyro activation test failed (error: %d)\n", ret);
    }
    */
}
