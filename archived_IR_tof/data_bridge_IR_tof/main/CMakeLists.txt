idf_component_register(SRCS "main.c"
                    REQUIRES esp_driver_i2c esp_wifi nvs_flash esp_event esp_driver_uart
                    INCLUDE_DIRS ".")
