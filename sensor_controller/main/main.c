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

// -------------------- ESP-NOW Section Below ---------------------

uint8_t peer_mac[6] = {0x3c,0x0f,0x02,0xe4,0x74,0x24}; // data_bridge
#define ESP_NOW_MASTER_NODE_ID 0
#define ESP_NOW_SELF_NODE_ID 1

void esp_now_recv_callback(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{

  if (data_len != sizeof(data_packet_t)) {
        ESP_LOGI(TAG,"received data : %.*s", data_len, data);
        return;
    }  

  data_packet_t data_packet_recv;
  memcpy(&data_packet_recv, data, sizeof(data_packet_t));

  ESP_LOGI(TAG, "Received data packet: node_id=%d, sensor_id= %d, trigger_shutter=%d, measure_distance=%d, distance_mm=%.1f", data_packet_recv.node_id, data_packet_recv.sensor_id, data_packet_recv.trigger_shutter, data_packet_recv.measure_distance, data_packet_recv.distance_mm / 10.0f);

  data_packet_t response_packet;
  if (data_packet_recv.node_id == ESP_NOW_MASTER_NODE_ID) {
    response_packet.node_id = ESP_NOW_SELF_NODE_ID;
    response_packet.sensor_id = data_packet_recv.sensor_id;
    response_packet.trigger_shutter = data_packet_recv.trigger_shutter;
    response_packet.measure_distance = data_packet_recv.measure_distance;
    response_packet.distance_mm = 0; // default to 0, will update if distance measurement is requested

    if (data_packet_recv.trigger_shutter)
    {
        ESP_LOGI(TAG, "Triggering camera shutter!");
        relay_all_on();
        vTaskDelay(pdMS_TO_TICKS(1000)); // Keep the pin high for 500 ms
        relay_all_off();
    }

    if (data_packet_recv.measure_distance) {
        urm14_t *sensor = &sensors[data_packet_recv.sensor_id - 1];

        uint16_t distance = urm14_read_distance(sensor);
        response_packet.distance_mm = distance;
    }
    esp_err_t err = esp_now_send(esp_now_info->src_addr, (uint8_t *)&response_packet, sizeof(response_packet));
  }
}

void wifi_sta_init(void)
{
    esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
  {
    nvs_flash_erase();
    ret = nvs_flash_init();
  }
  esp_netif_init();
  esp_event_loop_create_default();
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  esp_wifi_init(&cfg);
  esp_wifi_set_mode(WIFI_MODE_STA);
  esp_wifi_set_storage(WIFI_STORAGE_RAM);
  esp_wifi_set_ps(WIFI_PS_NONE);
  esp_wifi_start();
  esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);

  uint8_t my_esp_mac[6] = {};
  esp_read_mac(my_esp_mac, ESP_MAC_WIFI_STA);
//   ESP_LOGI(TAG, "my mac address " MACSTR "", my_esp_mac[0], my_esp_mac[1], my_esp_mac[2], my_esp_mac[3], my_esp_mac[4], my_esp_mac[5]);
}

void app_main(void)
{
    // Init relay board
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

    // Init ESP-NOW
    wifi_sta_init();
    esp_now_init();
    esp_now_register_recv_cb(esp_now_recv_callback);

    esp_now_peer_info_t peer_info = {0};
    peer_info.channel = 1; 
    peer_info.encrypt = false;

    memcpy(peer_info.peer_addr, peer_mac, 6);
    esp_now_add_peer(&peer_info);

    printf("READY_SERIAL\n");

    while(1)
    {

        // read from serial 
        int len = usb_serial_jtag_read_bytes(&data_packet_rx, sizeof(data_packet_t), 50 / portTICK_PERIOD_MS);

        if (len) {
            data_packet_tx.node_id = ESP_NOW_SELF_NODE_ID;
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