#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"

#include "esp_err.h"
#include "data_packet.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "mbcontroller.h"

#include "driver/gpio.h"

#include "urm14.h"
#include "relay_board.h"

static const char* TAG = "Data Acquisition";

void app_main(void)
{
    relay_board_init();

    // Init URM14's 
    for (size_t i = 0; i < sensor_count; i++)
    {
        urm14_t sensor = {
            .slave_addr = sensor_addresses[i]
        };

        urm14_init(&sensor);
    }
    
    while(1)
    {
        for (size_t i = 0; i < sensor_count; i++)
            {
                urm14_t sensor = {
                    .slave_addr = sensor_addresses[i]
                };

                uint16_t distance = urm14_read_distance(&sensor);
            }

        printf("Triggering all relays for 500 ms\n");
        relay_all_on();
        vTaskDelay(pdMS_TO_TICKS(500));
        relay_all_off();

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}