idf_component_register(
    SRCS 
        "src/i2c_manager.c"
    INCLUDE_DIRS 
                "src"
                "${CMAKE_CURRENT_SOURCE_DIR}/.."
                "${CMAKE_CURRENT_SOURCE_DIR}/../../"
    REQUIRES driver esp_driver_gpio
) 