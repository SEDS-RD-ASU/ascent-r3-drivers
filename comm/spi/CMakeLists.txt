idf_component_register(
    SRCS 
        "src/manual_spi_bus.c"
        "src/spi_manager.c"
    INCLUDE_DIRS
        "src"
        "${CMAKE_CURRENT_SOURCE_DIR}/../../"
    REQUIRES driver
) 