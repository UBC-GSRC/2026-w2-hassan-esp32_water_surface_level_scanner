#pragma once

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"
#include "esp_now.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t tx_ok;     /* send callbacks reporting SUCCESS (for broadcast: frame went out, not "was heard") */
    uint32_t tx_fail;
    uint32_t rx_count;  /* packets delivered to the rx callback */
} espnow_link_stats_t;

/* Called from the WiFi task. Keep it short: copy the data and post to a queue. */
typedef void (*espnow_link_rx_cb_t)(const uint8_t *src_mac, const uint8_t *data, int len, void *ctx);

/* Brings up NVS, netif, event loop, WiFi (STA) and ESP-NOW with a broadcast peer. */
esp_err_t espnow_link_init(uint8_t channel);

/* Tears ESP-NOW and WiFi down again. Safe to call after a failed init. */
void espnow_link_deinit(void);

/* Broadcast a packet (max ESP_NOW_MAX_DATA_LEN bytes). */
esp_err_t espnow_link_send(const void *data, size_t len);

/* Pass NULL to unregister. */
void espnow_link_set_rx_cb(espnow_link_rx_cb_t cb, void *ctx);

void espnow_link_get_stats(espnow_link_stats_t *out);
void espnow_link_reset_stats(void);

#ifdef __cplusplus
}
#endif
