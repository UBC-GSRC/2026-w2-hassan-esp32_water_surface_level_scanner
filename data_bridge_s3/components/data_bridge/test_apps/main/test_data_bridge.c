#include "unity.h"
#include "data_packet.h"
#include "driver/usb_serial_jtag.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BUF_SIZE (1024)

static void test_packet_size(void)
{
    TEST_ASSERT_EQUAL(
        6,
        sizeof(data_packet_t));
}


void test_serial_parsing(void)
{
    usb_serial_jtag_driver_config_t cfg = {
    .rx_buffer_size = BUF_SIZE,
    .tx_buffer_size = BUF_SIZE,
    };

    ESP_ERROR_CHECK(
        usb_serial_jtag_driver_install(&cfg));

    printf("READY_SERIAL");
    printf("\n");

    data_packet_t packet;

    int len = usb_serial_jtag_read_bytes(
        (uint8_t *)&packet,
        sizeof(packet),
        pdMS_TO_TICKS(10000));

    printf("Length: %u \n", len);

    if (len == sizeof(data_packet_t))
    {
        printf(
        "node=%u sensor=%u shutter=%u measure=%u distance=%u\n",
        packet.node_id,
        packet.sensor_id,
        packet.trigger_shutter,
        packet.measure_distance,
        packet.distance_mm);
    }
}

void app_main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_packet_size);
    RUN_TEST(test_serial_parsing);

    UNITY_END();
}