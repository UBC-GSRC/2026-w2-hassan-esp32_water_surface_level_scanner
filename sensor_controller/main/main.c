// Sensor node changes: the ESP-NOW callback only queues; a worker task does the slow work.
// Replace esp_now_recv_callback and the USB loop in app_main with this.
// Remove usb_serial_jtag_driver_install and the while(1) USB loop from app_main entirely.

#include "freertos/queue.h"

// Body carried inside the bridge's frame: seq + your existing packet.
typedef struct __attribute__((packed)) {
    uint8_t seq;
    data_packet_t pkt;
} wire_msg_t;

typedef struct {
    wire_msg_t msg;
} job_t;

static QueueHandle_t job_q;

void esp_now_send_cb(const wifi_tx_info_t *t, esp_now_send_status_t status)
{
    (void)t; (void)status; // no logging
}

void esp_now_recv_callback(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len != sizeof(wire_msg_t)) return;
    job_t j;
    memcpy(&j.msg, data, sizeof(wire_msg_t));
    xQueueSend(job_q, &j, 0); // never block in the Wi-Fi task
}

static void sensor_task(void *arg)
{
    job_t j;
    for (;;) {
        if (xQueueReceive(job_q, &j, portMAX_DELAY) != pdTRUE) continue;

        wire_msg_t rsp = {0};
        rsp.seq = j.msg.seq;                       // echo for the host
        rsp.pkt.node_id          = ESP_NOW_SELF_NODE_ID;
        rsp.pkt.sensor_id        = j.msg.pkt.sensor_id;
        rsp.pkt.trigger_shutter  = j.msg.pkt.trigger_shutter;
        rsp.pkt.measure_distance = j.msg.pkt.measure_distance;
        rsp.pkt.distance_mm      = 0;

        if (j.msg.pkt.trigger_shutter) relay_all_on(); else relay_all_off();

        if (j.msg.pkt.measure_distance) {
            uint8_t id = j.msg.pkt.sensor_id;
            if (id >= 1 && id <= 4) {
                rsp.pkt.distance_mm = urm14_read_distance(&sensors[id - 1]);
            } else {
                rsp.pkt.distance_mm = 0xFFFF; // invalid sensor id
            }
        }

        esp_now_send(peer_mac, (uint8_t *)&rsp, sizeof(rsp));
    }
}

// In app_main, after esp_now_init() and registering callbacks:
//   job_q = xQueueCreate(8, sizeof(job_t));
//   xTaskCreate(sensor_task, "sensor", 4096, NULL, 5, NULL);
