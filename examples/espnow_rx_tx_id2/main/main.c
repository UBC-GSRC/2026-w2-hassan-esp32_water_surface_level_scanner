#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"
static const char* TAG = "ESP-NOW RX TX Data Bridge";

void esp_now_send_callback(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
 
   ESP_LOGI(TAG,"Message sent");
}

void esp_now_recv_callback(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
 ESP_LOGI(TAG,"received data : %.*s", data_len, data);
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
  ESP_LOGI(TAG, "my mac address " MACSTR "", my_esp_mac[0], my_esp_mac[1], my_esp_mac[2], my_esp_mac[3], my_esp_mac[4], my_esp_mac[5]);
}

void app_main(void)
{
  wifi_sta_init();
  esp_now_init();
  esp_now_register_send_cb(esp_now_send_callback);
  esp_now_register_recv_cb(esp_now_recv_callback);

  esp_now_peer_info_t peer_info = {0};
  peer_info.channel = 1; 
  peer_info.encrypt = false;
  uint8_t peer_mac[6] = {0xd0,0xcf,0x13,0xe0,0xcb,0xc4}; // computer bridge

  memcpy(peer_info.peer_addr, peer_mac, 6);
  esp_now_add_peer(&peer_info);
  while(1)
  {
    // esp_err_t err = esp_now_send(esp_mac, (uint8_t *) "Sending via ESP-NOW", strlen("Sending via ESP-NOW"));
    const uint8_t message[] = "Message sent from data bridge";

    esp_err_t err = esp_now_send(peer_mac, message, strlen((char *)message));   
    ESP_LOGI(TAG,"esp now status : %s", esp_err_to_name(err));

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}