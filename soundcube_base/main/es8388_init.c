#include "esp_log.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "es8388";

#define I2C_SCL_GPIO  23
#define I2C_SDA_GPIO  18
#define ES8388_ADDR   0x10

#define ES8388_CONTROL1     0x00
#define ES8388_CONTROL2     0x01
#define ES8388_CHIPPOWER    0x02
#define ES8388_ADCPOWER     0x03
#define ES8388_DACPOWER     0x04
#define ES8388_MASTERMODE   0x08
#define ES8388_DACCONTROL1  0x17
#define ES8388_DACCONTROL2  0x18
#define ES8388_DACCONTROL3  0x19
#define ES8388_DACCONTROL4  0x1A
#define ES8388_DACCONTROL5  0x1B
#define ES8388_DACCONTROL16 0x26
#define ES8388_DACCONTROL17 0x27
#define ES8388_DACCONTROL20 0x2A
#define ES8388_DACCONTROL21 0x2B
#define ES8388_DACCONTROL23 0x2D
#define ES8388_DACCONTROL24 0x2E
#define ES8388_DACCONTROL25 0x2F
#define ES8388_DACCONTROL26 0x30
#define ES8388_DACCONTROL27 0x31

static i2c_master_bus_handle_t s_i2c_bus = NULL;
static i2c_master_dev_handle_t s_codec_dev = NULL;

static esp_err_t es8388_write_reg(uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(s_codec_dev, buf, sizeof(buf), pdMS_TO_TICKS(100));
}

esp_err_t es8388_init(void)
{
    i2c_master_bus_config_t bus_cfg = {
        .i2c_port = I2C_NUM_0,
        .sda_io_num = I2C_SDA_GPIO,
        .scl_io_num = I2C_SCL_GPIO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    esp_err_t ret = i2c_new_master_bus(&bus_cfg, &s_i2c_bus);
    if (ret != ESP_OK) return ret;

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = ES8388_ADDR,
        .scl_speed_hz = 100000,
    };
    ret = i2c_master_bus_add_device(s_i2c_bus, &dev_cfg, &s_codec_dev);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(50));

    es8388_write_reg(ES8388_DACCONTROL3, 0x04);
    es8388_write_reg(ES8388_CONTROL2, 0x50);
    es8388_write_reg(ES8388_CHIPPOWER, 0x00);
    es8388_write_reg(0x35, 0xA0);
    es8388_write_reg(0x37, 0xD0);
    es8388_write_reg(0x39, 0xD0);
    es8388_write_reg(ES8388_MASTERMODE, 0x00);
    es8388_write_reg(ES8388_DACPOWER, 0xC0);
    es8388_write_reg(ES8388_CONTROL1, 0x12);
    es8388_write_reg(ES8388_DACCONTROL1, 0x18);
    es8388_write_reg(ES8388_DACCONTROL2, 0x02);
    es8388_write_reg(ES8388_DACCONTROL16, 0x00);
    es8388_write_reg(ES8388_DACCONTROL17, 0x90);
    es8388_write_reg(ES8388_DACCONTROL20, 0x90);
    es8388_write_reg(ES8388_DACCONTROL21, 0x80);
    es8388_write_reg(ES8388_DACCONTROL23, 0x00);
    es8388_write_reg(ES8388_DACCONTROL4, 0x08);
    es8388_write_reg(ES8388_DACCONTROL5, 0x08);
    es8388_write_reg(ES8388_DACCONTROL24, 0x21);
    es8388_write_reg(ES8388_DACCONTROL25, 0x21);
    es8388_write_reg(ES8388_DACCONTROL26, 0x00);
    es8388_write_reg(ES8388_DACCONTROL27, 0x00);
    es8388_write_reg(ES8388_DACPOWER, 0x3C);
    es8388_write_reg(ES8388_CHIPPOWER, 0xF0);
    es8388_write_reg(ES8388_CHIPPOWER, 0x00);
    vTaskDelay(pdMS_TO_TICKS(100));
    es8388_write_reg(ES8388_DACCONTROL3, 0x00);

    ESP_LOGI(TAG, "ES8388 initialized successfully");
    return ESP_OK;
}

void es8388_unmute(void)
{
    es8388_write_reg(ES8388_DACCONTROL3, 0x00);
}