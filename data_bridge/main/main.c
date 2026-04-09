#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "driver/uart.h"
#include "data_packet.h"

#define uart_num UART_NUM_0
#define uart_buffer_size 1024

#define ESP_NOW_PEER_NODE_ID 1
#define ESP_NOW_SELF_NODE_ID 2
#define PEER_MAC_ADDRESS {0xd0,0xcf,0x13,0xe0,0xcb,0xc4} // Address of data acquisition esp32

uint8_t esp_mac[6];
static const char* TAG = "ESP-NOW RX";
void esp_now_recv_callback(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
//  ESP_LOGI(TAG,"received data : %.*s", data_len, data);

  uart_write_bytes(uart_num, (const char*)data, data_len);
  ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 100)); // wait timeout is 100 RTOS ticks (TickType_t)
  
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
  wifi_sta_init();
  esp_now_init();
  esp_now_register_recv_cb(esp_now_recv_callback);

  // UART Boilerplate
  // Setup UART buffered IO with event queue
    QueueHandle_t uart_queue;
    // Install UART driver using an event queue here
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

    const uart_port_t uart_num = UART_NUM_0;
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122,
    };
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(uart_num, &uart_config));

    // Configure ESP-NOW peer information
    data_packet_t res; 
    esp_now_peer_info_t peer_info = {0};
    peer_info.channel = 1; 
    peer_info.encrypt = false;

    uint8_t peer_mac[6] = {0xd0,0xcf,0x13,0xe0,0xcb,0xc4}; // computer bridge

    memcpy(peer_info.peer_addr, peer_mac, 6);
    esp_now_add_peer(&peer_info);

    while(1)
    {
        // wait for uart message 
        uart_read_bytes(uart_num, &res, sizeof(res), portMAX_DELAY); 
        
        // relay the uart message to the data acquisition esp32 via esp-now
        if (res.node_id == 0)
        {
          // uart_write_bytes(uart_num, (const char *)&res, sizeof(res));
          // ESP_ERROR_CHECK(uart_wait_tx_done(uart_num, 100)); // wait timeout is 100 RTOS ticks (TickType_t)
          esp_err_t err = esp_now_send(peer_mac, (uint8_t *)&res, sizeof(res));   

          vTaskDelay(pdMS_TO_TICKS(50)); // delay to prevent flooding the data acquisition esp32 with esp-now messages, which can cause packet loss
        }
        vTaskDelay(pdMS_TO_TICKS(10));
  }
}