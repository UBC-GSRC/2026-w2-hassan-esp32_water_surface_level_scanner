#include "relay_board.hpp"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "RELAY_TEST";

extern "C" void app_main(void)
{
    relay_board_init();

    while (true)
    {
        for (uint8_t relay = 1; relay <= 8; relay++)
        {
            ESP_LOGI(TAG, "Relay %u ON", relay);

            relay_set(relay, true);

            vTaskDelay(pdMS_TO_TICKS(500));

            ESP_LOGI(TAG, "Relay %u OFF", relay);

            relay_set(relay, false);

            vTaskDelay(pdMS_TO_TICKS(250));
        }

        ESP_LOGI(TAG, "Sweep complete");

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}