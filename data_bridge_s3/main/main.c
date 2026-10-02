// code a combination of https://esp32.com/viewtopic.php?t=43647, https://github.com/espressif/esp-idf/issues/11948#issuecomment-1653019021, https://github.com/espressif/esp-idf/issues/11948#issuecomment-1653019021
// The data bridge needs to do the following
// 1. Repeat all messages to ESPNOW peer(s) when received over USB serial connection
// 2. Repeat all messages received from ESPNOW peer(s) to USB serial connection 

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include "esp_system.h"
#include "esp_log.h"
#include "string.h"
#include "driver/usb_serial_jtag.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "data_packet.h" 

#define BUF_SIZE (1024)
#define ECHO_TASK_STACK_SIZE (4096)

uint8_t start_byte = 0xAF;
uint8_t peer_mac[6] = {0xac,0xa7,0x04,0x2e,0x21,0x78}; // Address of data acquisition esp32
uint8_t esp_mac[6];
static const char* TAG = "ESP-NOW RX";

void esp_now_recv_callback(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
  usb_serial_jtag_write_bytes(&start_byte, 1, 20 / portTICK_PERIOD_MS);  
  usb_serial_jtag_write_bytes((const char *) data, data_len, 20 / portTICK_PERIOD_MS);  
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

  esp_read_mac(esp_mac, ESP_MAC_WIFI_STA);

  // NOTE: Uncomment to get the MAC address of the ESP32 that is running this firmware
  // ESP_LOGI(TAG, "peer mac " MACSTR "", esp_mac[0], esp_mac[1], esp_mac[2], esp_mac[3], esp_mac[4], esp_mac[5]);
}

void app_main(void)
{
    // Turn off logging 
    esp_log_level_set("*", ESP_LOG_NONE);

    // Configure ESP-NOW peer information
    wifi_sta_init();
    esp_now_init();
    esp_now_register_recv_cb(esp_now_recv_callback);

    esp_now_peer_info_t peer_info = {0};
    peer_info.channel = 1; 
    peer_info.encrypt = false;

    memcpy(peer_info.peer_addr, peer_mac, 6);
    esp_now_add_peer(&peer_info);

    // Configure USB SERIAL JTAG
    usb_serial_jtag_driver_config_t usb_serial_jtag_config = {
        .rx_buffer_size = BUF_SIZE,
        .tx_buffer_size = BUF_SIZE,
    };

    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&usb_serial_jtag_config));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    if (data == NULL) {
        ESP_LOGE("usb_serial_jtag echo", "no memory for data");
        return;
    }

    printf("READY_SERIAL\n");

    uint8_t start;
    while (1) {

        if (usb_serial_jtag_read_bytes(&start, 1, 20 / portTICK_PERIOD_MS) == 1)
        {
            if (start == start_byte) {
                int len = usb_serial_jtag_read_bytes(data, (BUF_SIZE), 20 / portTICK_PERIOD_MS);
                // Forward message from serial jtag to esp now peer
                if (len == sizeof(data_packet_t)) {
                    esp_err_t err = esp_now_send(peer_mac, (uint8_t *)data, len);
                    vTaskDelay(pdMS_TO_TICKS(50));
                }
            }
        }


    }
}
