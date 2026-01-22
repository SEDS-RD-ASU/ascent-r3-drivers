#ifndef ONBOARD_LED_H
#define ONBOARD_LED_H

#include "led_strip.h"
#include "esp_log.h"
#include "esp_err.h"
#define TAG "onboard_led"

#define LED_STRIP_LED_COUNT 1
#define LED_STRIP_MEMORY_BLOCK_WORDS 1024 // this determines the DMA block size
#define LED_STRIP_RMT_RES_HZ  (10 * 1000 * 1000)


esp_err_t led_init(int pin);
esp_err_t led_set_color(uint8_t r, uint8_t g, uint8_t b);
esp_err_t led_red(void);
esp_err_t led_green(void);
esp_err_t led_blue(void);
esp_err_t led_yellow(void);
esp_err_t led_off(void);

#endif