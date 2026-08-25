#include "relay_board.h"
#include "urm14.h"
#include "sensor_config.h"

#include "esp_log.h"

static const char *TAG = "HW_TEST";

static urm14_t sensors[16];

static void pass(const char *test)
{
    ESP_LOGI(TAG, "[PASS] %s", test);
}

static void fail(const char *test)
{
    ESP_LOGE(TAG, "[FAIL] %s", test);
}

static void test_distance_sensor(
    size_t index,
    urm14_t *sensor)
{
    uint16_t distance =
        urm14_read_distance(sensor);

    if (distance > 0)
    {
        ESP_LOGI(
            TAG,
            "Sensor %u (0x%02X): %u mm",
            (unsigned)(index + 1),
            sensor->slave_addr,
            distance);

        pass("URM14");
    }
    else
    {
        fail("URM14");
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

    /*
     * Build sensor objects from config
     */
    for (size_t i = 0; i < sensor_count; i++)
    {
        sensors[i].slave_addr =
            sensor_addresses[i];

        urm14_init(&sensors[i]);
    }

    /*
     * Test all configured sensors
     */
    for (size_t i = 0; i < sensor_count; i++)
    {
        test_distance_sensor(
            i,
            &sensors[i]);
    }

    ESP_LOGI(TAG, "==================================");
    ESP_LOGI(TAG, "TESTS COMPLETE");
    ESP_LOGI(TAG, "==================================");
}