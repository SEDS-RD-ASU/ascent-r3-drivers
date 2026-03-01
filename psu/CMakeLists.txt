idf_component_register(
    SRCS 
        "src/driver_psu.c"
        "examples/psu_test.c"
    INCLUDE_DIRS 
        "src"
        "examples"
        "${CMAKE_CURRENT_SOURCE_DIR}/../"
    REQUIRES driver adc esp_adc
) 