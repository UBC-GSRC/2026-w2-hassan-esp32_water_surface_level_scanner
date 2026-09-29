/* Data bridge firmware: thin app_main on top of the shared espnow_link component. */
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "espnow_link.h"

#define ESPNOW_CHANNEL 1

static const char *TAG = "data_bridge";

typedef struct {
    uint8_t src_mac[ESP_NOW_ETH_ALEN];
    uint8_t len;
    uint8_t data[ESP_NOW_MAX_DATA_LEN];
} rx_item_t;

static QueueHandle_t s_rx_queue;

static void on_rx(const uint8_t *src_mac, const uint8_t *data, int len, void *ctx)
{
    rx_item_t item = { .len = len };
    memcpy(item.src_mac, src_mac, ESP_NOW_ETH_ALEN);
    memcpy(item.data, data, len);
    xQueueSend(s_rx_queue, &item, 0);
}

void app_main(void)
{
    s_rx_queue = xQueueCreate(16, sizeof(rx_item_t));
    ESP_ERROR_CHECK(espnow_link_init(ESPNOW_CHANNEL));
    espnow_link_set_rx_cb(on_rx, NULL);

    rx_item_t item;
    while (xQueueReceive(s_rx_queue, &item, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI(TAG, "rx %u bytes from " MACSTR, item.len, MAC2STR(item.src_mac));
        /* TODO: forward to host over USB serial */
    }
}