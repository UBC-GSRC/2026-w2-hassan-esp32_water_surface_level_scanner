// Sensor controller (data acquisition node)
// ESP-NOW receive callback only queues a job. A worker task does the relay and sensor
// work and sends the response. No USB-Serial-JTAG driver is installed here, so the USB
// port carries only the console log.

#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_mac.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_now.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "mbcontroller.h"

#include "data_packet.h"
#include "urm14.h"
#include "relay_board.h"

static const char *TAG = "Data Acquisition";

#define ESP_NOW_MASTER_NODE_ID 0
#define ESP_NOW_SELF_NODE_ID   1
#define MAX_SENSOR_ID        4
#define DISTANCE_ERROR       0xFFFF

static urm14_t sensors[MAX_SENSOR_ID];

// MAC address of the data bridge
static const uint8_t peer_mac[6] = {0x3c, 0x0f, 0x02, 0xe4, 0x74, 0xc24};

// Body carried inside the bridge's frame: seq + packet. The bridge forwards it untouched.
typedef struct __attribute__((packed)) {
    uint8_t seq;
    data_packet_t pkt;
} wire_msg_t;

_Static_assert(sizeof(wire_msg_t) == 7, "wire_msg_t must be 7 bytes (1 seq + 6 packet)");

typedef struct {
    wire_msg_t msg;
} job_t;

static QueueHandle_t job_q;

// -------------------- ESP-NOW --------------------

static void esp_now_send_cb(const wifi_tx_info_t *t, esp_now_send_status_t status)
{
    (void)t;
    (void)status; // keep the Wi-Fi task free of logging
}

static void esp_now_recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len != sizeof(wire_msg_t)) {
        return;
    }

    job_t j;
    memcpy(&j.msg, data, sizeof(wire_msg_t));

    // Never block in the Wi-Fi task: drop if the queue is full (host will retry)
    xQueueSend(job_q, &j, 0);
}

static void wifi_sta_init(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
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
}

// -------------------- Worker task --------------------

static void sensor_task(void *arg)
{
    job_t j;

    for (;;) {
        if (xQueueReceive(job_q, &j, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        const data_packet_t *req = &j.msg.pkt;

        // Only act on requests from the master node (node_id 0)
        if (req->node_id != ESP_NOW_MASTER_NODE_ID) {
            continue;
        }

        ESP_LOGI(TAG, "REQ seq=%u sensor=%u shutter=%u measure=%u",
                 j.msg.seq, req->sensor_id, req->trigger_shutter, req->measure_distance);

        wire_msg_t rsp = {0};
        rsp.seq                  = j.msg.seq; // echo so the host can match the reply
        rsp.pkt.node_id          = ESP_NOW_SELF_NODE_ID;
        rsp.pkt.sensor_id        = req->sensor_id;
        rsp.pkt.trigger_shutter  = req->trigger_shutter;
        rsp.pkt.measure_distance = req->measure_distance;
        rsp.pkt.distance_mm      = 0;

        if (req->trigger_shutter) {
            ESP_LOGI(TAG, "Triggering camera shutter");
            relay_all_on();
        } else {
            relay_all_off();
        }

        if (req->measure_distance) {
            uint8_t id = req->sensor_id;
            if (id >= 1 && id <= MAX_SENSOR_ID) {
                rsp.pkt.distance_mm = urm14_read_distance(&sensors[id - 1]);
            } else {
                ESP_LOGW(TAG, "Invalid sensor_id %u", id);
                rsp.pkt.distance_mm = DISTANCE_ERROR;
            }
        }

        esp_err_t err = esp_now_send(peer_mac, (const uint8_t *)&rsp, sizeof(rsp));
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "esp_now_send failed: %s", esp_err_to_name(err));
        }
    }
}

// -------------------- app_main --------------------

void app_main(void)
{
    // Relay board
    relay_board_init();

    URM14 sensors, initialized in place
    for (size_t i = 0; i < sensor_count && i < MAX_SENSOR_ID; i++) {
        sensors[i] = (urm14_t){ .slave_addr = sensor_addresses[i] };
        urm14_init(&sensors[i]);
    }

    // Job queue and worker task must exist before ESP-NOW can deliver messages
    job_q = xQueueCreate(8, sizeof(job_t));
    configASSERT(job_q != NULL);
    xTaskCreate(sensor_task, "sensor_task", 4096, NULL, 5, NULL);

    // ESP-NOW
    wifi_sta_init();
    ESP_ERROR_CHECK(esp_now_init());
    esp_now_register_recv_cb(esp_now_recv_callback);
    esp_now_register_send_cb(esp_now_send_cb);

    esp_now_peer_info_t peer_info = {0};
    peer_info.channel = 1;
    peer_info.encrypt = false;
    memcpy(peer_info.peer_addr, peer_mac, 6);
    ESP_ERROR_CHECK(esp_now_add_peer(&peer_info));

    ESP_LOGI(TAG, "READY");
    // Nothing else to do: all work happens in sensor_task
}
