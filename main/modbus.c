#include "modbus.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#define MODBUS_UART     UART_NUM_1
#define TXD_PIN         16
#define RXD_PIN         17
#define DE_RE_PIN       23
#define MODBUS_SLAVE_ADDR 1

static const char *TAG = "Modbus";

static const uint16_t crc_table[] = {
    0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
    0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400};

static uint16_t crc16_modbus(uint8_t *data, int len)
{
    uint16_t crc = 0xFFFF;
    for (int i = 0; i < len; i++)
    {
        uint16_t tbl_idx = (crc ^ data[i]) & 0x0F;
        crc = (crc >> 4) ^ crc_table[tbl_idx];
        tbl_idx = (crc ^ (data[i] >> 4)) & 0x0F;
        crc = (crc >> 4) ^ crc_table[tbl_idx];
    }
    return crc;
}

static float bytes_to_float(uint8_t *bytes)
{
    uint32_t val = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
    return *(float *)&val;
}

void modbus_uart_init(void)
{
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    uart_param_config(MODBUS_UART, &uart_config);
    uart_set_pin(MODBUS_UART, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(MODBUS_UART, 256, 0, 0, NULL, 0);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << DE_RE_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);
    gpio_set_level(DE_RE_PIN, 0); // Start in RX mode
}

esp_err_t modbus_read_float_register(uint16_t reg_addr, float *result)
{
    uart_flush_input(MODBUS_UART);
    vTaskDelay(pdMS_TO_TICKS(5));

    uint8_t request[8];
    request[0] = MODBUS_SLAVE_ADDR;
    request[1] = 0x04;
    request[2] = (reg_addr >> 8) & 0xFF;
    request[3] = reg_addr & 0xFF;
    request[4] = 0x00;
    request[5] = 0x02;

    uint16_t crc = crc16_modbus(request, 6);
    request[6] = crc & 0xFF;
    request[7] = (crc >> 8) & 0xFF;

    gpio_set_level(DE_RE_PIN, 1);
    vTaskDelay(pdMS_TO_TICKS(2));

    uart_write_bytes(MODBUS_UART, request, 8);
    uart_wait_tx_done(MODBUS_UART, pdMS_TO_TICKS(100));

    vTaskDelay(pdMS_TO_TICKS(5));

    gpio_set_level(DE_RE_PIN, 0);
    vTaskDelay(pdMS_TO_TICKS(2));

    uint8_t response[9];
    int len = uart_read_bytes(MODBUS_UART, response, 9, pdMS_TO_TICKS(1000));

    if (len != 9)
    {
        ESP_LOGE(TAG, "No response (got %d bytes)", len);
        return ESP_FAIL;
    }

    uint16_t response_crc = crc16_modbus(response, 7);
    uint16_t received_crc = response[7] | (response[8] << 8);

    if (response_crc != received_crc)
    {
        ESP_LOGE(TAG, "CRC mismatch");
        return ESP_FAIL;
    }

    *result = bytes_to_float(&response[3]);
    return ESP_OK;
}
