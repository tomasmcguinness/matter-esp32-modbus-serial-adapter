#pragma once

#include "esp_err.h"
#include <stdint.h>

void modbus_uart_init(void);
esp_err_t modbus_read_float_register(uint16_t reg_addr, float *result);
