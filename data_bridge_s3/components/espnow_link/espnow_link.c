#include <string.h>
#include "espnow_link.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "espnow_link";

static const uint8_t BROADCAST_MAC[ESP_NOW_ETH_ALEN] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static espnow_link_rx_cb_t s_rx_cb;
static void *s_rx_ctx;
static volatile uint32_t s_tx_ok, s_tx_fail, s_rx_count;
static uint8_t s_channel;

/* ESP-IDF 5.5+ signature. On older IDF this is (const uint8_t *mac_addr, esp_now_send_status_t status). */
static void send_cb(const esp_now_send_info_t *tx_info, esp_now_send_status_t status)
{
    if (status == ESP_NOW_SEND_SUCCESS) s_tx_ok++;
    else                                s_tx_fail++;
}

static void recv_cb(const esp_now_recv_info_t *info, const uint8_t *data, int len)
{
    if (info == NULL || data == NULL || len <= 0) return;
    s_rx_count++;
    if (s_rx_cb) s_rx_cb(info->src_addr, data, len, s_rx_ctx);
}

esp_err_t espnow_link_init(uint8_t channel)
{
    s_channel = channel;

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "nvs erase");
        err = nvs_flash_init();
    }
    ESP_RETURN_ON_ERROR(err, TAG, "nvs init");

    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "netif init");
    err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {   /* already created: fine */
        return err;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "wifi init");
    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "wifi storage");
    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "wifi mode");
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "wifi start");
    ESP_RETURN_ON_ERROR(esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE), TAG, "wifi channel");

    ESP_RETURN_ON_ERROR(esp_now_init(), TAG, "espnow init");
    ESP_RETURN_ON_ERROR(esp_now_register_send_cb(send_cb), TAG, "send cb");
    ESP_RETURN_ON_ERROR(esp_now_register_recv_cb(recv_cb), TAG, "recv cb");

    esp_now_peer_info_t peer = {0};
    peer.channel = channel;
    peer.ifidx   = WIFI_IF_STA;
    peer.encrypt = false;
    memcpy(peer.peer_addr, BROADCAST_MAC, ESP_NOW_ETH_ALEN);
    ESP_RETURN_ON_ERROR(esp_now_add_peer(&peer), TAG, "add peer");

    return ESP_OK;
}

void espnow_link_deinit(void)
{
    /* Errors ignored on purpose so this works after a partial init. */
    esp_now_deinit();
    esp_wifi_stop();
    esp_wifi_deinit();
}

esp_err_t espnow_link_send(const void *data, size_t len)
{
    return esp_now_send(BROADCAST_MAC, (const uint8_t *)data, len);
}

void espnow_link_set_rx_cb(espnow_link_rx_cb_t cb, void *ctx)
{
    s_rx_ctx = ctx;
    s_rx_cb = cb;
}

void espnow_link_get_stats(espnow_link_stats_t *out)
{
    out->tx_ok = s_tx_ok;
    out->tx_fail = s_tx_fail;
    out->rx_count = s_rx_count;
}

void espnow_link_reset_stats(void)
{
    s_tx_ok = s_tx_fail = s_rx_count = 0;
}