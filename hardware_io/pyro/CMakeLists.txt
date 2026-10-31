idf_component_register(
    SRCS 
        "src/driver_pyro.c"
        "examples/pyro_test.c"
    INCLUDE_DIRS 
        "src"
        "examples"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../"
    REQUIRES driver adc psu esp_timer esp_driver_gpio
) 