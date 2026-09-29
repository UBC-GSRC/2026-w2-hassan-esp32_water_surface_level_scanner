idf_component_register(
    SRCS "test_data_bridge.c"
    INCLUDE_DIRS "."
    REQUIRES unity espnow_link esp_wifi
)