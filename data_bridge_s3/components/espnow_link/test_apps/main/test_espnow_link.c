/* ESP-NOW link tests. One binary, flashed to BOTH boards: no per-device role.
 *
 * The receive test is symmetric: each board beacons while listening, and any packet carrying
 * TEST_MAGIC that arrives counts as "heard the peer" (a board never receives its own broadcast).
 */
#include <stdio.h>
#include <string.h>
#include "unity.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "espnow_link.h"

#define ESPNOW_CHANNEL      1
#define TEST_MAGIC          0xE5
#define SEND_PERIOD_MS      200
#define SEND_TEST_COUNT     25
#define RX_TARGET_COUNT     5
#define RX_TIMEOUT_MS       30000   /* covers the boards booting at different times */

typedef struct __attribute__((packed)) {
    uint8_t  magic;
    uint32_t seq;
} test_packet_t;

static volatile uint32_t s_magic_rx;
static uint8_t s_peer_mac[ESP_NOW_ETH_ALEN];

static void on_rx(const uint8_t *src_mac, const uint8_t *data, int len, void *ctx)
{
    if (len != sizeof(test_packet_t) || data[0] != TEST_MAGIC) return;
    memcpy(s_peer_mac, src_mac, ESP_NOW_ETH_ALEN);
    s_magic_rx++;
}

/* Unity runs these around every RUN_TEST, so each test starts from a clean link. */
void setUp(void)
{
    s_magic_rx = 0;
    espnow_link_reset_stats();
    espnow_link_set_rx_cb(on_rx, NULL);
}

void tearDown(void)
{
    espnow_link_set_rx_cb(NULL, NULL);
    espnow_link_deinit();
}

/* ---------------- tests ---------------- */

void test_espnow_init(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, espnow_link_init(ESPNOW_CHANNEL));

    uint8_t mac[6];
    TEST_ASSERT_EQUAL(ESP_OK, esp_read_mac(mac, ESP_MAC_WIFI_STA));
    printf("ESPNOW_INIT_OK mac=" MACSTR "\n", MAC2STR(mac));
}

void test_espnow_init_deinit_cycle(void)
{
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL(ESP_OK, espnow_link_init(ESPNOW_CHANNEL));
        espnow_link_deinit();
    }
    /* tearDown deinits again; that must be harmless */
}

void test_espnow_send(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, espnow_link_init(ESPNOW_CHANNEL));

    test_packet_t pkt = { .magic = TEST_MAGIC };
    for (int i = 0; i < SEND_TEST_COUNT; i++) {
        pkt.seq = i;
        TEST_ASSERT_EQUAL(ESP_OK, espnow_link_send(&pkt, sizeof(pkt)));
        vTaskDelay(pdMS_TO_TICKS(SEND_PERIOD_MS));
    }
    vTaskDelay(pdMS_TO_TICKS(100));   /* let the last send callback land */

    espnow_link_stats_t st;
    espnow_link_get_stats(&st);
    printf("ESPNOW_SEND ok=%lu fail=%lu\n", (unsigned long)st.tx_ok, (unsigned long)st.tx_fail);
    TEST_ASSERT_EQUAL_UINT32(SEND_TEST_COUNT, st.tx_ok);
    TEST_ASSERT_EQUAL_UINT32(0, st.tx_fail);
}

void send_magic_espnow(void){
    espnow_link_init(ESPNOW_CHANNEL);
    printf("SENDING A FINAL FEW MESSAGES\n\n");
    test_packet_t pkt = { .magic = TEST_MAGIC };
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(RX_TIMEOUT_MS);

    vTaskDelay(pdMS_TO_TICKS(5000));

    for (int i=0; i < 10; i++){
        espnow_link_send(&pkt, sizeof(pkt));
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}

void test_espnow_receive(void)
{
    TEST_ASSERT_EQUAL(ESP_OK, espnow_link_init(ESPNOW_CHANNEL));

    printf("READY_ESPNOW_RX\n");

    test_packet_t pkt = { .magic = TEST_MAGIC };
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(RX_TIMEOUT_MS);

    while (s_magic_rx < RX_TARGET_COUNT && xTaskGetTickCount() < deadline) {
        espnow_link_send(&pkt, sizeof(pkt));
        pkt.seq++;
        vTaskDelay(pdMS_TO_TICKS(SEND_PERIOD_MS));
    }

    printf("ESPNOW_RX peer_packets=%lu peer_mac=" MACSTR "\n",
           (unsigned long)s_magic_rx, MAC2STR(s_peer_mac));
    TEST_ASSERT_GREATER_OR_EQUAL_UINT32(RX_TARGET_COUNT, s_magic_rx);
}

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_espnow_init);
    RUN_TEST(test_espnow_init_deinit_cycle);
    RUN_TEST(test_espnow_send);
    RUN_TEST(test_espnow_receive);
    send_magic_espnow();
    UNITY_END();
}