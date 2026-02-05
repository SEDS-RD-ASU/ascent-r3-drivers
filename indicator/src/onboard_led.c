#include "onboard_led.h"

static led_strip_handle_t led_strip = NULL;

static led_strip_handle_t configure_led(int pin)
{
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRB, // The color order of the strip: GRB
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: RMT
    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,        // different clock source can lead to different power consumption
        .resolution_hz = LED_STRIP_RMT_RES_HZ, // RMT counter clock frequency
        .mem_block_symbols = LED_STRIP_MEMORY_BLOCK_WORDS, // the memory block size used by the RMT channel
        .flags = {
            .with_dma = 1,     // Using DMA can improve performance when driving more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with RMT backend");
    return led_strip;
}

esp_err_t led_init(int pin)
{
    // Configure the LED strip GPIO pin
    led_strip = configure_led(pin);
    if (led_strip == NULL) {
        ESP_LOGE(TAG, "Failed to initialize LED strip");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "LED strip initialized on GPIO %d", pin);
    return ESP_OK;
}

esp_err_t led_set_color(uint8_t r, uint8_t g, uint8_t b)
{
    if (led_strip == NULL) {
        ESP_LOGE(TAG, "LED strip not initialized");
        return ESP_FAIL;
    }
    // Set the color of the LED
    ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, 0, r, g, b));
    // Refresh the strip to send data
    ESP_ERROR_CHECK(led_strip_refresh(led_strip));
    return ESP_OK;
}

esp_err_t led_red(void)
{
    return led_set_color(255, 0, 0);
}

esp_err_t led_green(void)
{
    return led_set_color(0, 255, 0);
}

esp_err_t led_blue(void)
{
    return led_set_color(0, 0, 255);
}

esp_err_t led_yellow(void)
{
    return led_set_color(255, 255, 0);
}

esp_err_t led_purple(void)
{
    return led_set_color(255, 0, 255);
}

esp_err_t led_off(void)
{
    return led_strip_clear(led_strip);
}