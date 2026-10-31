idf_component_register(
    SRCS 
        "src/uart_manager.c"
    INCLUDE_DIRS
        "src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../"
    REQUIRES driver esp_driver_uart esp_driver_gpio
) 