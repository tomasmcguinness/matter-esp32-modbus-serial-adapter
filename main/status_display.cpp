#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "status_display.h"

#include "driver/i2c_master.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lvgl_port.h"

#include "lvgl.h"

static const char *TAG = "status_display";

#define I2C_BUS_PORT 0

#define EXAMPLE_LCD_PIXEL_CLOCK_HZ (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA        22
#define EXAMPLE_PIN_NUM_SCL        23
#define EXAMPLE_PIN_NUM_RST        2
#define EXAMPLE_I2C_HW_ADDR        0x3C

#define EXAMPLE_LCD_CMD_BITS   8
#define EXAMPLE_LCD_PARAM_BITS 8

#define EXAMPLE_LCD_H_RES 128
#define EXAMPLE_LCD_V_RES 64

StatusDisplay StatusDisplay::sStatusDisplay;

esp_err_t StatusDisplay::Init()
{
    ESP_LOGI(TAG, "StatusDisplay::Init()");

    ESP_LOGI(TAG, "Initialize I2C bus");
    i2c_master_bus_handle_t i2c_bus = NULL;
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_BUS_PORT,
        .sda_io_num = (gpio_num_t)EXAMPLE_PIN_NUM_SDA,
        .scl_io_num = (gpio_num_t)EXAMPLE_PIN_NUM_SCL,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags = {
            .enable_internal_pullup = true,
        }};
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t io_config = {
        .dev_addr = EXAMPLE_I2C_HW_ADDR,
        .control_phase_bytes = 1,
        .dc_bit_offset = 6,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_CMD_BITS,
        .scl_speed_hz = EXAMPLE_LCD_PIXEL_CLOCK_HZ,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_i2c(i2c_bus, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install SSD1306 panel driver");
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_PIN_NUM_RST,
        .bits_per_pixel = 1,
    };

    esp_lcd_panel_ssd1306_config_t ssd1306_config = {
        .height = EXAMPLE_LCD_V_RES,
    };
    panel_config.vendor_config = &ssd1306_config;
    ESP_ERROR_CHECK(esp_lcd_new_panel_ssd1306(io_handle, &panel_config, &mPanelHandle));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(mPanelHandle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(mPanelHandle));
    esp_lcd_panel_io_tx_param(io_handle, 0x81, (uint8_t[]){0xFF}, 1); // Max contrast

    ESP_LOGI(TAG, "Initialize LVGL");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_port_init(&lvgl_cfg);

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = mPanelHandle,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_V_RES,
        .double_buffer = true,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .monochrome = true,
        .rotation = {
            .swap_xy = false,
            .mirror_x = false,
            .mirror_y = false,
        }};

    mDisplayHandle = lvgl_port_add_disp(&disp_cfg);

    lv_disp_set_rotation(mDisplayHandle, LV_DISP_ROT_180);

    lv_obj_t *scr = lv_scr_act();

    mVoltageLabel = lv_label_create(scr);
    lv_label_set_text(mVoltageLabel, "V: --");
    lv_obj_set_width(mVoltageLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mVoltageLabel, LV_ALIGN_TOP_LEFT, 0, 0);

    mCurrentLabel = lv_label_create(scr);
    lv_label_set_text(mCurrentLabel, "A: --");
    lv_obj_set_width(mCurrentLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mCurrentLabel, LV_ALIGN_TOP_LEFT, 0, 16);

    mPowerLabel = lv_label_create(scr);
    lv_label_set_text(mPowerLabel, "W: --");
    lv_obj_set_width(mPowerLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mPowerLabel, LV_ALIGN_TOP_LEFT, 0, 32);

    mEnergyLabel = lv_label_create(scr);
    lv_label_set_text(mEnergyLabel, "kWh: --");
    lv_obj_set_width(mEnergyLabel, mDisplayHandle->driver->hor_res);
    lv_obj_align(mEnergyLabel, LV_ALIGN_TOP_LEFT, 0, 48);

    ESP_LOGI(TAG, "StatusDisplay::Init() finished");

    return ESP_OK;
}

void StatusDisplay::TurnOn()
{
    ESP_LOGI(TAG, "Turning display on");
    esp_lcd_panel_disp_on_off(mPanelHandle, true);
}

void StatusDisplay::TurnOff()
{
    ESP_LOGI(TAG, "Turning display off");
    esp_lcd_panel_disp_on_off(mPanelHandle, false);
}

extern "C" void status_display_update(float voltage, float current, float power, float energy)
{
    StatusDisplayMgr().UpdateDisplay(voltage, current, power, energy);
}

void StatusDisplay::UpdateDisplay(float voltage, float current, float power, float energy)
{
    ESP_LOGI(TAG, "Updating the display");

    char buf[32];

    snprintf(buf, sizeof(buf), "V: %.1f", voltage);
    lv_label_set_text(mVoltageLabel, buf);

    snprintf(buf, sizeof(buf), "A: %.2f", current);
    lv_label_set_text(mCurrentLabel, buf);

    snprintf(buf, sizeof(buf), "W: %.1f", power);
    lv_label_set_text(mPowerLabel, buf);

    snprintf(buf, sizeof(buf), "kWh: %.2f", energy);
    lv_label_set_text(mEnergyLabel, buf);
}
