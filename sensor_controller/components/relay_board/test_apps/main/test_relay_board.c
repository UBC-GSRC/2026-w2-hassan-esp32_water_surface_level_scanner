#include "unity.h"
#include "relay_board.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static void test_relay_board_init(void){
    TEST_ASSERT_TRUE(relay_board_init());
}

static void test_relay_1(void){
    TEST_ASSERT_TRUE(relay_set(1, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void test_relay_2(void){
    TEST_ASSERT_TRUE(relay_set(2, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void test_relay_3(void){
    TEST_ASSERT_TRUE(relay_set(3, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void test_relay_4(void){
    TEST_ASSERT_TRUE(relay_set(4, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void test_relay_5(void){
    TEST_ASSERT_TRUE(relay_set(5, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void test_relay_6(void){
    TEST_ASSERT_TRUE(relay_set(6, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void test_relay_7(void){
    TEST_ASSERT_TRUE(relay_set(7, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

static void test_relay_8(void){
    TEST_ASSERT_TRUE(relay_set(8, true));
    vTaskDelay(pdMS_TO_TICKS(500));
}

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_relay_board_init);
    RUN_TEST(test_relay_1);
    RUN_TEST(test_relay_2);
    RUN_TEST(test_relay_3);
    RUN_TEST(test_relay_4);
    RUN_TEST(test_relay_5);
    RUN_TEST(test_relay_6);
    RUN_TEST(test_relay_7);
    RUN_TEST(test_relay_8);

    UNITY_END();
}