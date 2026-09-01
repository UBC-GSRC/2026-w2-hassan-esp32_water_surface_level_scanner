#include "urm14.h"

#include "esp_log.h"
#include "esp_err.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/uart.h"
#include "mbcontroller.h"

static const char *TAG = "URM14";

#define MB_UART_PORT   UART_NUM_1
#define MB_BAUDRATE    19200

#define MB_TXD         17
#define MB_RXD         18
#define MB_RTS         UART_PIN_NO_CHANGE

#define PUBLIC_ADDR    0x00

#define REG_ADDR       2
#define REG_DISTANCE   5
#define REG_CONTROL    8

#define TEMP_CPT_SEL_BIT       (1 << 0)
#define TEMP_CPT_ENABLE_BIT    (1 << 1)
#define MEASURE_MODE_BIT       (1 << 2)
#define MEASURE_TRIG_BIT       (1 << 3)

static void *mb_handle = NULL;
static bool modbus_initialized = false;

const uint16_t sensor_addresses[] = {
    0x01,
    0x02,
    0x03,
    0x04
};

const size_t sensor_count =
    sizeof(sensor_addresses) /
    sizeof(sensor_addresses[0]);

static void modbus_master_init(void)
{
    if (modbus_initialized)
    {
        return;
    }

    mb_communication_info_t comm = {
        .ser_opts.port = MB_UART_PORT,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = MB_BAUDRATE,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
        .ser_opts.response_tout_ms = 3000
    };

    ESP_ERROR_CHECK(
        mbc_master_create_serial(
            &comm,
            &mb_handle));

    ESP_ERROR_CHECK(
        uart_set_pin(
            MB_UART_PORT,
            MB_TXD,
            MB_RXD,
            MB_RTS,
            UART_PIN_NO_CHANGE));

    ESP_ERROR_CHECK(
        uart_set_mode(
            MB_UART_PORT,
            UART_MODE_RS485_HALF_DUPLEX));

    ESP_ERROR_CHECK(
        mbc_master_start(
            mb_handle));

    modbus_initialized = true;

    ESP_LOGI(TAG, "Modbus RTU master initialized");
}

static esp_err_t write_register(
    uint16_t slave_addr,
    uint16_t reg,
    uint16_t value)
{
    mb_param_request_t req = {
        .slave_addr = slave_addr,
        .command = 0x06,
        .reg_start = reg,
        .reg_size = 1
    };

    return mbc_master_send_request(
        mb_handle,
        &req,
        &value);
}

static esp_err_t read_distance_register(
    uint16_t slave_addr,
    uint16_t *value)
{
    mb_param_request_t req = {
        .slave_addr = slave_addr,
        .command = 0x03,
        .reg_start = REG_DISTANCE,
        .reg_size = 1
    };

    return mbc_master_send_request(
        mb_handle,
        &req,
        value);
}

bool urm14_init(urm14_t *sensor)
{
    modbus_master_init();

    sensor->control_register = 0;

    sensor->control_register |= MEASURE_MODE_BIT;
    sensor->control_register &= ~TEMP_CPT_SEL_BIT;
    sensor->control_register &= ~TEMP_CPT_ENABLE_BIT;

    ESP_ERROR_CHECK(
        write_register(
            sensor->slave_addr,
            REG_CONTROL,
            sensor->control_register));

    vTaskDelay(pdMS_TO_TICKS(100));

    ESP_LOGI(
        TAG,
        "URM14 configured at address 0x%02X",
        sensor->slave_addr);

    return true;
}

uint16_t urm14_read_distance(urm14_t *sensor)
{
    sensor->control_register |= MEASURE_TRIG_BIT;

    if (write_register(
            sensor->slave_addr,
            REG_CONTROL,
            sensor->control_register) != ESP_OK)
    {
        ESP_LOGW(
            TAG,
            "Failed to trigger measurement at address 0x%02X",
            sensor->slave_addr);

        return 0;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint16_t raw_distance = 0;

    if (read_distance_register(
            sensor->slave_addr,
            &raw_distance) == ESP_OK)
    {
        ESP_LOGI(
            TAG,
            "Address 0x%02X distance = %.1f mm (raw=%u)",
            sensor->slave_addr,
            raw_distance / 10.0f,
            raw_distance);

        return raw_distance;
    }

    ESP_LOGW(
        TAG,
        "Read distance failed at address 0x%02X",
        sensor->slave_addr);

    return 0;
}

bool urm14_self_test(urm14_t *sensor)
{
    urm14_init(sensor);

    uint16_t distance =
        urm14_read_distance(sensor);

    if (distance == 0)
    {
        ESP_LOGE(
            TAG,
            "[FAIL] URM14 0x%02X",
            sensor->slave_addr);

           return false;
    }

    ESP_LOGI(
        TAG,
        "[PASS] URM14 0x%02X",
        sensor->slave_addr);

    return true;
}

bool urm14_set_address(
    urm14_t *sensor,
    uint16_t new_address)
{
    modbus_master_init();

    esp_err_t err =
        write_register(
            PUBLIC_ADDR,
            REG_ADDR,
            new_address);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to set address to 0x%02X (%s)",
            new_address,
            esp_err_to_name(err));

        return false;
    }

    ESP_LOGI(
        TAG,
        "Address changed to 0x%02X",
        new_address);

    sensor->slave_addr = new_address;

    return true;
}
