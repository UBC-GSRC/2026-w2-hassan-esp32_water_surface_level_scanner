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

static const char* TAG = "Data Acquisition";

// ------------------ Camera Section Below -------------------
// from https://github.com/espressif/esp-idf/blob/v6.0/examples/peripherals/gpio/generic_gpio/main/gpio_example_main.c

#define CAMERA_PIN 8
#define GPIO_OUTPUT_PIN_SEL 1ULL << CAMERA_PIN

void init_camera_gpio(void)
{
  gpio_config_t io_conf = {};
  //disable interrupt
  io_conf.intr_type = GPIO_INTR_DISABLE;
  //set as output mode
  io_conf.mode = GPIO_MODE_OUTPUT;
  //bit mask of the pins that you want to set,e.g.GPIO18/19
  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
  //disable pull-down mode
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  //disable pull-up mode
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  //configure GPIO with the given settings
  gpio_config(&io_conf);
}

// ------------------ URM14 Section Below -------------------
/* UART / RS485 */
#define MB_UART_PORT   UART_NUM_1
#define MB_BAUDRATE    19200

#define MB_TXD  23
#define MB_RXD  24
#define MB_RTS  25

/* URM14 Modbus */
#define SLAVE_ADDR     0x0C

/* URM14 registers (0-based for Modbus) */
#define REG_DISTANCE   5   // eDistance
#define REG_CONTROL    8   // eControl

/* Control register bits (same as Arduino) */
#define TEMP_CPT_SEL_BIT       (1 << 0)
#define TEMP_CPT_ENABLE_BIT    (1 << 1)
#define MEASURE_MODE_BIT       (1 << 2)
#define MEASURE_TRIG_BIT       (1 << 3)

/* Modbus handle */
static void *mb_handle = NULL;

/* Persistent control register (same as Arduino `cr`) */
static uint16_t cr = 0;

/* ---------------- Modbus Init ---------------- */

static void modbus_master_init(void)
{
    mb_communication_info_t comm = {
        .ser_opts.port = MB_UART_PORT,
        .ser_opts.mode = MB_RTU,
        .ser_opts.baudrate = MB_BAUDRATE,
        .ser_opts.parity = MB_PARITY_NONE,
        .ser_opts.data_bits = UART_DATA_8_BITS,
        .ser_opts.stop_bits = UART_STOP_BITS_1,
        .ser_opts.response_tout_ms = 3000
    };

    ESP_ERROR_CHECK(mbc_master_create_serial(&comm, &mb_handle));
    ESP_ERROR_CHECK(uart_set_pin(
        MB_UART_PORT,
        MB_TXD,
        MB_RXD,
        MB_RTS,
        UART_PIN_NO_CHANGE
    ));
    ESP_ERROR_CHECK(uart_set_mode(
        MB_UART_PORT,
        UART_MODE_RS485_HALF_DUPLEX
    ));
    ESP_ERROR_CHECK(mbc_master_start(mb_handle));

    ESP_LOGI(TAG, "Modbus RTU master initialized");
}

/* ---------------- Modbus helpers ---------------- */

static esp_err_t write_control(uint16_t value)
{
    mb_param_request_t req = {
        .slave_addr = SLAVE_ADDR,
        .command    = 0x06,          // Write Single Register
        .reg_start  = REG_CONTROL,
        .reg_size   = 1
    };

    return mbc_master_send_request(mb_handle, &req, &value);
}

static esp_err_t read_distance(uint16_t *value)
{
    mb_param_request_t req = {
        .slave_addr = SLAVE_ADDR,
        .command    = 0x03,          // Read Holding Registers
        .reg_start  = REG_DISTANCE,
        .reg_size   = 1
    };

    return mbc_master_send_request(mb_handle, &req, value);
}

uint16_t read_urm14_distance(void)
{
    cr |= MEASURE_TRIG_BIT;             // Set trigger bit
    write_control(cr);                  // Trigger ranging

    vTaskDelay(pdMS_TO_TICKS(50));     // Wait for ranging
    uint16_t raw_distance;

    if (read_distance(&raw_distance) == ESP_OK) {
        float dist_mm = raw_distance / 10.0f;
        ESP_LOGI(TAG, "distance = %.1f mm (raw=%u)",
                  dist_mm, raw_distance);
        return raw_distance;
    } else {
        ESP_LOGW(TAG, "Read distance failed");
        return 0;
    }
}

void init_urm14(void)
{
    modbus_master_init();

    cr |= MEASURE_MODE_BIT;                 // Trigger mode
    cr &= ~TEMP_CPT_SEL_BIT;                // Internal temp
    cr &= ~TEMP_CPT_ENABLE_BIT;             // ENABLE temp compensation

    write_control(cr);                      // Same as Arduino setup()
    vTaskDelay(pdMS_TO_TICKS(100));

    // ESP_LOGI(TAG, "URM14 configured");
}


// -------------------- ESP-NOW Section Below ---------------------

#define ESP_NOW_MASTER_NODE_ID 0
#define ESP_NOW_SELF_NODE_ID 1
#define PEER_MAC_ADDR {0xd0,0xcf,0x13,0xe0,0xec,0xa4};

void esp_now_send_callback(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
 
   ESP_LOGI(TAG,"Message sent");
}

void esp_now_recv_callback(const esp_now_recv_info_t * esp_now_info, const uint8_t *data, int data_len)
{

  if (data_len != sizeof(data_packet_t)) {
        ESP_LOGI(TAG,"received data : %.*s", data_len, data);
        return;
    }  

  data_packet_t data_packet_recv;
  memcpy(&data_packet_recv, data, sizeof(data_packet_t));

  ESP_LOGI(TAG, "Received data packet: node_id=%d, trigger_shutter=%d, measure_distance=%d, distance_mm=%.1f", data_packet_recv.node_id, data_packet_recv.trigger_shutter, data_packet_recv.measure_distance, data_packet_recv.distance_mm / 10.0f);

  data_packet_t response_packet;
  if (data_packet_recv.node_id == ESP_NOW_MASTER_NODE_ID) {
    response_packet.node_id = ESP_NOW_SELF_NODE_ID;
    response_packet.trigger_shutter = data_packet_recv.trigger_shutter;
    response_packet.measure_distance = data_packet_recv.measure_distance;
    response_packet.distance_mm = 0; // default to 0, will update if distance measurement is requested

    if (data_packet_recv.trigger_shutter && gpio_get_level(CAMERA_PIN) == 0)
    {
        ESP_LOGI(TAG, "Triggering camera shutter!");
        gpio_set_level(CAMERA_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(500)); // Keep the pin high for 500 ms
        gpio_set_level(CAMERA_PIN, 0);
    }

    if (data_packet_recv.measure_distance) {
        response_packet.distance_mm = read_urm14_distance();
        ESP_LOGI(TAG, "Distance measurement requested, current distance = %.1f mm", response_packet.distance_mm / 10.0f);
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
  ESP_LOGI(TAG, "my mac address " MACSTR "", my_esp_mac[0], my_esp_mac[1], my_esp_mac[2], my_esp_mac[3], my_esp_mac[4], my_esp_mac[5]);
}

void app_main(void)
{
  wifi_sta_init();
  esp_now_init();
  esp_now_register_send_cb(esp_now_send_callback);
  esp_now_register_recv_cb(esp_now_recv_callback);

  init_urm14();
  init_camera_gpio();

  esp_now_peer_info_t peer_info = {0};
  peer_info.channel = 1; 
  peer_info.encrypt = false;
  uint8_t peer_mac[6] = {0xd0,0xcf,0x13,0xe0,0xec,0xa4}; // data_bridge

  memcpy(peer_info.peer_addr, peer_mac, 6);
  esp_now_add_peer(&peer_info);

  while(1)
  {
    // esp_err_t err = esp_now_send(esp_mac, (uint8_t *) "Sending via ESP-NOW", strlen("Sending via ESP-NOW"));
    // const uint8_t message[] = "Message sent from data acquisition";

    // NOTE: uncomment below to send distance measurements periodically without waiting for a request
    // data_packet_t data_packet = {
    //   .node_id = ESP_NOW_SELF_NODE_ID,
    //   .trigger_shutter = false,
    //   .measure_distance = true,
    //   .distance_mm = read_urm14_distance() // example distance in mm (123.4 cm)
    // };
    // esp_err_t err = esp_now_send(peer_mac, (uint8_t *)&data_packet, sizeof(data_packet));   
    // ESP_LOGI(TAG,"esp now status : %s", esp_err_to_name(err));

    
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}