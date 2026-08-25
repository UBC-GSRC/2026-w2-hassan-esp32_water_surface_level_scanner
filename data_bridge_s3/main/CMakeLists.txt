idf_component_register(SRCS "main.c"
                    REQUIRES esp_driver_uart esp_wifi nvs_flash esp_event esp_netif driver
                    INCLUDE_DIRS "." "../../type_definitions")
