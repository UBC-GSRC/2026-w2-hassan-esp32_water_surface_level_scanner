#include "relay_board.h"
#include "urm14.h"

#include "esp_log.h"

static const char *TAG = "HW_TEST";

static urm14_t sensor = {
    .slave_addr = 0x11
};

static void pass(const char *test)
{
    ESP_LOGI(TAG, "[PASS] %s", test);
}

static void fail(const char *test)
{
    ESP_LOGE(TAG, "[FAIL] %s", test);
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

    if (urm14_self_test(&sensor))
    {
        pass("URM14");
    }
    else
    {
        fail("URM14");
    }

    ESP_LOGI(TAG, "==================================");
    ESP_LOGI(TAG, "TESTS COMPLETE");
    ESP_LOGI(TAG, "==================================");
}