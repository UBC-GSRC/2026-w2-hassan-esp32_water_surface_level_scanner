#include <stdio.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "string.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "driver/uart.h"

#define uart_num UART_NUM_0
#define uart_buffer_size 1024

uint8_t esp_mac[6];
static const char* TAG = "ESP-NOW RX";
void esp_now_recv_callback(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{
//  ESP_LOGI(TAG,"received data : %.*s", data_len, data);

  char msg[uart_buffer_size];

  // Format the message just like ESP_LOGI
  int len = snprintf(msg, sizeof(msg),
                      "%.*s\r\n",
                      data_len, data);

  uart_write_bytes(uart_num, (const char*)msg, strlen(msg));
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
  ESP_LOGI(TAG, "peer mac " MACSTR "", esp_mac[0], esp_mac[1], esp_mac[2], esp_mac[3], esp_mac[4], esp_mac[5]);
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

    // Set UART pins(TX: IO4, RX: IO5, RTS: IO18, CTS: IO19)
    // ESP_ERROR_CHECK(uart_set_pin(UART_NUM_1, 4, 5, 18, 19));
  

    while(1)
    {
      // Write data to UART.
      
      // Write data to UART, end with a break signal.
      // uart_write_bytes_with_break(uart_num, "test break\n",strlen("test break\n"), 100);
  
      // Wait for packet to be sent
      // const uart_port_t uart_num = UART_NUM_0;
  
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}