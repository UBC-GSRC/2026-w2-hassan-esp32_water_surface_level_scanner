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
#include "driver/usb_serial_jtag.h"

#include "urm14.h"
#include "relay_board.h"
#include "data_packet.h"

static const char* TAG = "Data Acquisition";
data_packet_t data_packet_rx;
data_packet_t data_packet_tx;

#define BUF_SIZE (1024)

static urm14_t sensors[4];
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

        sensors[i] = sensor;
    }
    
    // Init USB Serial JTAG

    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));
    // ESP_LOGI("usb_serial_jtag echo", "USB_SERIAL_JTAG init done");


    while(1)
    {

        // read from serial 
        int len = usb_serial_jtag_read_bytes(&data_packet_rx, sizeof(data_packet_t), 50 / portTICK_PERIOD_MS);

        if (len) {
            data_packet_tx.node_id = 1;
            data_packet_tx.sensor_id = data_packet_rx.sensor_id;
            data_packet_tx.trigger_shutter = data_packet_rx.trigger_shutter;
            data_packet_tx.measure_distance = data_packet_rx.measure_distance;
            data_packet_tx.distance_mm = 0;

            if (data_packet_rx.trigger_shutter){
                relay_all_on();
                vTaskDelay(pdMS_TO_TICKS(1000));
                relay_all_off(); 
            }

            if (data_packet_rx.measure_distance) {
                urm14_t sensor = sensors[data_packet_rx.sensor_id - 1];

                uint16_t distance = urm14_read_distance(&sensor);
                data_packet_tx.distance_mm = distance;
                vTaskDelay(pdMS_TO_TICKS(100));
            }

            usb_serial_jtag_write_bytes(&data_packet_tx, sizeof(data_packet_t), 50 / portTICK_PERIOD_MS);

        }

        // for (size_t i = 0; i < sensor_count; i++)
        //     {
        //         urm14_t sensor = {
        //             .slave_addr = sensor_addresses[i]
        //         };

        //         uint16_t distance = urm14_read_distance(&sensor);
        //         printf("Sensor %d, distance: %.1f mm\n", sensor_addresses[i], distance / 10.0f);
        //     }

        // printf("Triggering all relays for 500 ms\n");
        // relay_all_on();
        // vTaskDelay(pdMS_TO_TICKS(500));
        // relay_all_off();

        // vTaskDelay(pdMS_TO_TICKS(1000));
        vTaskDelay(pdMS_TO_TICKS(100));

    }
}