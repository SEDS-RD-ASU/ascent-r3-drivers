idf_component_register(
    SRCS 
        "src/driver_buzzer.c"
        "src/beep.c"
        "examples/buzzer_test.c"
        "src/onboard_led.c"
    INCLUDE_DIRS 
        "src"
        "examples"
        "${CMAKE_CURRENT_SOURCE_DIR}/../"
    PRIV_REQUIRES esp_pm led_strip
    REQUIRES driver adc psu esp_driver_ledc esp_driver_gpio
) 