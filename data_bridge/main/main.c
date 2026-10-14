// Data bridge: USB-Serial-JTAG <-> ESP-NOW, framed + CRC protocol.
// Frame: [0xAF][len][body (len bytes)][crc8 over len+body]
// The bridge treats the body as opaque bytes and never needs data_packet_t.
//
// menuconfig: set the console to UART or NONE so logs never share the USB data channel.

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_now.h"
#include "nvs_flash.h"
#include "driver/usb_serial_jtag.h"

#define SOF        0xAF
#define MAX_BODY   32
#define RX_CHUNK   64

static const uint8_t peer_mac[6] = {0x14, 0xc1, 0x9f, 0x20, 0x3b, 0xc4}; // sensor node

typedef struct {
    uint8_t len;
    uint8_t body[MAX_BODY];
} msg_t;

static QueueHandle_t to_host_q;

static uint8_t crc8(const uint8_t *d, size_t n)
{
    uint8_t c = 0;
    while (n--) {
        c ^= *d++;
        for (int i = 0; i < 8; i++) {
            c = (c & 0x80) ? (uint8_t)((c << 1) ^ 0x07) : (uint8_t)(c << 1);
        }
    }
    return c;
}

// ---------------- ESP-NOW ----------------

static void on_send(const wifi_tx_info_t *t, esp_now_send_status_t s)
{
    (void)t; (void)s; // no logging here
}

static void on_recv(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (len <= 0 || len > MAX_BODY) return;
    msg_t m = { .len = (uint8_t)len };
    memcpy(m.body, data, len);
    xQueueSend(to_host_q, &m, 0); // never block in the Wi-Fi task
}

static void wifi_init(void)
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

// ---------------- Device -> host ----------------

static void host_tx_task(void *arg)
{
    msg_t m;
    uint8_t frame[MAX_BODY + 3];
    for (;;) {
        if (xQueueReceive(to_host_q, &m, portMAX_DELAY) != pdTRUE) continue;
        frame[0] = SOF;
        frame[1] = m.len;
        memcpy(&frame[2], m.body, m.len);
        frame[2 + m.len] = crc8(&frame[1], 1 + m.len);
        // One write per frame. Result ignored on purpose: the host retries.
        usb_serial_jtag_write_bytes(frame, 3 + m.len, pdMS_TO_TICKS(100));
    }
}

// ---------------- Host -> device parser ----------------

typedef enum { S_SOF, S_LEN, S_BODY, S_CRC } state_t;
static state_t st = S_SOF;
static uint8_t blen, bidx, body[MAX_BODY];

static void feed(uint8_t b)
{
    switch (st) {
    case S_SOF:
        if (b == SOF) st = S_LEN;
        break;
    case S_LEN:
        if (b == 0 || b > MAX_BODY) {
            st = (b == SOF) ? S_LEN : S_SOF; // resync
        } else {
            blen = b; bidx = 0; st = S_BODY;
        }
        break;
    case S_BODY:
        body[bidx++] = b;
        if (bidx == blen) st = S_CRC;
        break;
    case S_CRC: {
        uint8_t tmp[MAX_BODY + 1];
        tmp[0] = blen;
        memcpy(&tmp[1], body, blen);
        if (crc8(tmp, blen + 1) == b) {
            esp_now_send(peer_mac, body, blen);
        }
        st = S_SOF;
        break;
    }
    }
}

void app_main(void)
{
    esp_log_level_set("*", ESP_LOG_NONE);

    wifi_init();
    esp_now_init();
    esp_now_register_recv_cb(on_recv);
    esp_now_register_send_cb(on_send);

    esp_now_peer_info_t peer = {0};
    peer.channel = 1;
    peer.encrypt = false;
    memcpy(peer.peer_addr, peer_mac, 6);
    esp_now_add_peer(&peer);

    to_host_q = xQueueCreate(16, sizeof(msg_t));
    configASSERT(to_host_q);

    usb_serial_jtag_driver_config_t cfg = {
        .rx_buffer_size = 1024,
        .tx_buffer_size = 1024,
    };
    ESP_ERROR_CHECK(usb_serial_jtag_driver_install(&cfg));

    xTaskCreate(host_tx_task, "host_tx", 4096, NULL, 5, NULL);

    uint8_t buf[RX_CHUNK];
    TickType_t last_rx = xTaskGetTickCount();

    for (;;) {
        int n = usb_serial_jtag_read_bytes(buf, sizeof(buf), pdMS_TO_TICKS(20));
        if (n > 0) {
            last_rx = xTaskGetTickCount();
            for (int i = 0; i < n; i++) feed(buf[i]);
        } else if (st != S_SOF &&
                   (xTaskGetTickCount() - last_rx) > pdMS_TO_TICKS(50)) {
            st = S_SOF; // abandon a half-received frame
        }
    }
}
