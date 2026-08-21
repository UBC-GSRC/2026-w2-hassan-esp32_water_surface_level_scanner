#include "relay_board.h"
#include "urm14.h"

#include "esp_log.h"

static const char *TAG = "HW_TEST";

static urm14_t front_sensor = {
    .slave_addr = 0x01
};

static urm14_t left_sensor = {
    .slave_addr = 0x02
};

static urm14_t right_sensor = {
    .slave_addr = 0x03
};

static urm14_t rear_sensor = {
    .slave_addr = 0x04
};

static void pass(const char *test)
{
    ESP_LOGI(TAG, "[PASS] %s", test);
}

static void fail(const char *test)
{
    ESP_LOGE(TAG, "[FAIL] %s", test);
}

static void test_distance_sensor(
    const char *name,
    urm14_t *sensor)
{
    uint16_t distance =
        urm14_read_distance(sensor);

    if (distance > 0)
    {
        ESP_LOGI(
            TAG,
            "%s (0x%02X): %u mm",
            name,
            sensor->slave_addr,
            distance);

        pass(name);
    }
    else
    {
        fail(name);
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "==================================");
    ESP_LOGI(TAG, "STARTING HARDWARE TESTS");
    ESP_LOGI(TAG, "==================================");

    if (relay_board_self_test())
    {
        pass("Relay Board");
    }
    else
    {
        fail("Relay Board");
    }

    urm14_init(&front_sensor);
    urm14_init(&left_sensor);
    urm14_init(&right_sensor);
    urm14_init(&rear_sensor);

    test_distance_sensor(
        "Front URM14",
        &front_sensor);

    test_distance_sensor(
        "Left URM14",
        &left_sensor);

    test_distance_sensor(
        "Right URM14",
        &right_sensor);

    test_distance_sensor(
        "Rear URM14",
        &rear_sensor);

    ESP_LOGI(TAG, "==================================");
    ESP_LOGI(TAG, "TESTS COMPLETE");
    ESP_LOGI(TAG, "==================================");
}