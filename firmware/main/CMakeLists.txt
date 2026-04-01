idf_component_register(SRCS "usb_serial.cpp" "sdm120.cpp" "modbus.cpp" "electrical_power_measurement_delegate.cpp" "status_display.cpp" "main.cpp"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
