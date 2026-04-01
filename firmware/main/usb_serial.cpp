#include "usb_serial.h"

#include <string.h>
#include <esp_log.h>
#include <driver/usb_serial_jtag.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static const char *TAG = "USBSerial";

#define USB_SERIAL_BUF_SIZE 1024

static void usb_serial_read_task(void *arg)
{
    uint8_t buf[USB_SERIAL_BUF_SIZE];
    int pos = 0;

    while (true)
    {
        int len = usb_serial_jtag_read_bytes(buf + pos, sizeof(buf) - pos - 1, pdMS_TO_TICKS(100));

        if (len > 0)
        {
            pos += len;
            buf[pos] = '\0';

            // Look for newline delimiter
            char *newline = strchr((char *)buf, '\n');
            while (newline != NULL)
            {
                *newline = '\0';

                // Trim trailing \r if present
                if (newline > (char *)buf && *(newline - 1) == '\r')
                {
                    *(newline - 1) = '\0';
                }

                if (strlen((char *)buf) > 0)
                {
                    ESP_LOGI(TAG, "Received JSON: %s", (char *)buf);
                }

                // Shift remaining data to front of buffer
                int remaining = pos - (int)(newline - (char *)buf + 1);
                if (remaining > 0)
                {
                    memmove(buf, newline + 1, remaining);
                }
                pos = remaining;
                buf[pos] = '\0';

                newline = strchr((char *)buf, '\n');
            }

            // If buffer is full with no newline, log and reset
            if (pos >= (int)(sizeof(buf) - 1))
            {
                buf[pos] = '\0';
                ESP_LOGW(TAG, "Buffer overflow, discarding: %s", (char *)buf);
                pos = 0;
            }
        }
    }
}

void usb_serial_init(void)
{
    usb_serial_jtag_driver_config_t config = {
        .tx_buffer_size = USB_SERIAL_BUF_SIZE,
        .rx_buffer_size = USB_SERIAL_BUF_SIZE,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&config));
    xTaskCreate(usb_serial_read_task, "usb_serial", 4096, NULL, 5, NULL);
    ESP_LOGI(TAG, "USB serial listener started");
}
