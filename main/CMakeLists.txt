idf_component_register(SRCS "electrical_power_measurement_delegate.cpp" "status_display.cpp" "main.cpp" "modbus.c" "sdm120.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
