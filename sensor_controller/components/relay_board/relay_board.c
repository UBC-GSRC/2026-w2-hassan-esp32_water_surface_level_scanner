#include "relay_board.h"

#include "driver/i2c_master.h"
#include "esp_check.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TCA9554_ADDR 0x20

#define I2C_SDA GPIO_NUM_42
#define I2C_SCL GPIO_NUM_41

static i2c_master_dev_handle_t device_handle = NULL;

static uint8_t relay_state = 0xFF;

static void write_outputs(void)
{
    uint8_t data[] = {
        0x01,
        relay_state
    };

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            device_handle,
            data,
            sizeof(data),
            -1));
}

bool relay_board_init(void)
{
    i2c_master_bus_handle_t bus_handle;

    i2c_master_bus_config_t bus_cfg = {0};

    bus_cfg.i2c_port = I2C_NUM_0;
    bus_cfg.sda_io_num = I2C_SDA;
    bus_cfg.scl_io_num = I2C_SCL;
    bus_cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    bus_cfg.glitch_ignore_cnt = 7;

    ESP_ERROR_CHECK(
        i2c_new_master_bus(
            &bus_cfg,
            &bus_handle));

    i2c_device_config_t dev_cfg = {0};

    dev_cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    dev_cfg.device_address = TCA9554_ADDR;
    dev_cfg.scl_speed_hz = 100000;

    ESP_ERROR_CHECK(
        i2c_master_bus_add_device(
            bus_handle,
            &dev_cfg,
            &device_handle));

    uint8_t config_data[] = {
        0x03,
        0x00
    };

    ESP_ERROR_CHECK(
        i2c_master_transmit(
            device_handle,
            config_data,
            sizeof(config_data),
            -1));

    relay_all_off();

    return true;
}

bool relay_all_off(void)
{
    relay_state = 0xFF;
    write_outputs();

    return true;
}

bool relay_set(uint8_t relay, bool on)
{
    if (relay < 1 || relay > 8)
    {
        return false;
    }

    uint8_t bit = relay - 1;

    if (on)
    {
        relay_state &= ~(1U << bit);
    }
    else
    {
        relay_state |= (1U << bit);
    }

    write_outputs();

    return true;
}

bool relay_board_self_test(void)
{
    relay_board_init();

    for (uint8_t relay = 1; relay <= 8; relay++)
    {
        relay_set(relay, true);
        vTaskDelay(pdMS_TO_TICKS(500));

        relay_set(relay, false);
        vTaskDelay(pdMS_TO_TICKS(250));
    }
    return true;
}