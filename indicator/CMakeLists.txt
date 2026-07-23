idf_component_register(
    SRCS 
        "src/driver_buzzer.c"
        "src/beep.c"
    INCLUDE_DIRS 
        "src"
        "examples"
        "${CMAKE_CURRENT_SOURCE_DIR}/../"
    PRIV_REQUIRES esp_driver_ledc esp_driver_gpio esp_pm
    REQUIRES driver adc psu 
) 