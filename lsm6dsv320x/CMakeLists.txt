idf_component_register(
    SRCS 
        "src/driver_LSM6DSV320X.c"
        "interface/interface_LSM6DSV320X.c"
    INCLUDE_DIRS 
        "src"
        "interface"
        "${CMAKE_CURRENT_SOURCE_DIR}/.."

    REQUIRES driver spi
) 