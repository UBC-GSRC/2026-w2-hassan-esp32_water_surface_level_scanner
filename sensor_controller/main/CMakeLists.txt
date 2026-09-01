idf_component_register(
    SRCS "main.c"
    REQUIRES esp_wifi nvs_flash esp_event esp_netif relay_board urm14 sensor_config
    INCLUDE_DIRS "." "../../type_definitions"
)
