#include "unity.h"

#include "urm14.h"

static void test_urm14_init_multiple(void)
{
    TEST_ASSERT_LESS_OR_EQUAL(16, sensor_count);

    for (size_t i = 0; i < sensor_count; i++)
    {
        urm14_t sensor = {
            .slave_addr = sensor_addresses[i]
        };

        TEST_ASSERT_TRUE(
            urm14_init(&sensor));
    }
}

static void test_urm14_read_distance_multiple(void)
{
    TEST_ASSERT_LESS_OR_EQUAL(16, sensor_count);

    for (size_t i = 0; i < sensor_count; i++)
    {
        urm14_t sensor = {
            .slave_addr = sensor_addresses[i]
        };

        TEST_ASSERT_TRUE(
            urm14_init(&sensor));

        uint16_t distance =
            urm14_read_distance(&sensor);

        TEST_ASSERT_GREATER_THAN(
            0,
            distance);
    }
}

static void test_urm14_init_single(void)
{
    urm14_t sensor = {
        .slave_addr = sensor_addresses[0]
    };

    TEST_ASSERT_TRUE(
        urm14_init(&sensor));
}

static void test_urm14_read_distance_single(void)
{
    urm14_t sensor = {
        .slave_addr = sensor_addresses[0]
    };

    TEST_ASSERT_TRUE(
        urm14_init(&sensor));

    uint16_t distance =
        urm14_read_distance(&sensor);

    TEST_ASSERT_GREATER_THAN(
        0,
        distance);
}

void setUp(void)
{
}

void tearDown(void)
{
}

void app_main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_urm14_init_single);
    RUN_TEST(test_urm14_read_distance_single);

    RUN_TEST(test_urm14_init_multiple);
    RUN_TEST(test_urm14_read_distance_multiple);
    printf("DONE.");
    
    UNITY_END();
}