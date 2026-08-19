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

#define SLAVE_ADDR     0x11

#define REG_DISTANCE   5
#define REG_CONTROL    8

#define TEMP_CPT_SEL_BIT       (1 << 0)
#define TEMP_CPT_ENABLE_BIT    (1 << 1)
#define MEASURE_MODE_BIT       (1 << 2)
#define MEASURE_TRIG_BIT       (1 << 3)

#define PUBLIC_ADDR   0x00

#define REG_PID       0
#define REG_VID       1
#define REG_ADDR      2
#define REG_BAUD      3
#define REG_PARITY    4
#define REG_DISTANCE  5
#define REG_INT_TEMP  6
#define REG_EXT_TEMP  7
#define REG_CONTROL   8
#define REG_NOISE     9

static void *mb_handle = NULL;
static uint16_t cr = 0;
static bool initialized = false;

static void modbus_master_init(void)
{
    mb_communication_info_t comm = {
        .ser_opts.port = MB_UART_PORT,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = MB_BAUDRATE,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
        .ser_opts.response_tout_ms = 3000
    };

    ESP_LOGI(TAG, "Creating Modbus master");

    ESP_ERROR_CHECK(
        mbc_master_create_serial(
            &comm,
            &mb_handle));

    ESP_LOGI(TAG, "mb_handle=%p", mb_handle);

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

    esp_err_t err = mbc_master_start(mb_handle);

    ESP_LOGI(
        TAG,
        "mbc_master_start() = %s",
        esp_err_to_name(err));

    ESP_ERROR_CHECK(err);

    ESP_LOGI(TAG, "Modbus RTU master initialized");
}

static esp_err_t write_control(uint16_t value)
{
    mb_param_request_t req = {
        .slave_addr = SLAVE_ADDR,
        .command = 0x06,
        .reg_start = REG_CONTROL,
        .reg_size = 1
    };

    return mbc_master_send_request(
        mb_handle,
        &req,
        &value);
}

static esp_err_t read_distance_register(uint16_t *value)
{
    mb_param_request_t req = {
        .slave_addr = SLAVE_ADDR,
        .command = 0x03,
        .reg_start = REG_DISTANCE,
        .reg_size = 1
    };

    return mbc_master_send_request(
        mb_handle,
        &req,
        value);
}

void urm14_init(void)
{
    if (initialized)
    {
        return;
    }

    modbus_master_init();

    cr |= MEASURE_MODE_BIT;
    cr &= ~TEMP_CPT_SEL_BIT;
    cr &= ~TEMP_CPT_ENABLE_BIT;

    ESP_ERROR_CHECK(write_control(cr));

    vTaskDelay(pdMS_TO_TICKS(100));

    initialized = true;

    ESP_LOGI(TAG, "URM14 configured");
}

uint16_t urm14_read_distance(void)
{
    if (!initialized)
    {
        ESP_LOGE(TAG, "URM14 not initialized");
        return 0;
    }

    cr |= MEASURE_TRIG_BIT;

    if (write_control(cr) != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to trigger measurement");
        return 0;
    }

    vTaskDelay(pdMS_TO_TICKS(50));

    uint16_t raw_distance = 0;

    if (read_distance_register(&raw_distance) == ESP_OK)
    {
        ESP_LOGI(
            TAG,
            "distance = %.1f mm (raw=%u)",
            raw_distance / 10.0f,
            raw_distance);

        return raw_distance;
    }

    ESP_LOGW(TAG, "Read distance failed");

    return 0;
}

bool urm14_self_test(void)
{
    ESP_LOGI(TAG, "Starting URM14 self test");

    urm14_init();

    uint16_t distance = urm14_read_distance();

    if (distance == 0)
    {
        ESP_LOGE(TAG, "[FAIL] URM14");
        return false;
    }

    ESP_LOGI(
        TAG,
        "[PASS] URM14 distance = %.1f mm",
        distance / 10.0f);

    return true;
}

bool urm14_set_address(uint16_t new_address)
{
    if (!initialized)
    {
        urm14_init();
    }

    mb_param_request_t req = {
        .slave_addr = PUBLIC_ADDR,
        .command = 0x06,
        .reg_start = REG_ADDR,
        .reg_size = 1
    };

    esp_err_t err = mbc_master_send_request(
        mb_handle,
        &req,
        &new_address);

    if (err != ESP_OK)
    {
        ESP_LOGE(
            TAG,
            "Failed to change address to 0x%02X (%s)",
            new_address,
            esp_err_to_name(err));

        return false;
    }

    ESP_LOGI(
        TAG,
        "Address changed to 0x%02X",
        new_address);

    ESP_LOGI(
        TAG,
        "Power-cycle or reset the sensor for the new address to take effect");

    return true;
}