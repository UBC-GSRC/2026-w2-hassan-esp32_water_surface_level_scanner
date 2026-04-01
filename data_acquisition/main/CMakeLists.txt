idf_component_register(SRCS "esp_now_tx.c" "main.c" 
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_i2c esp_wifi nvs_flash esp_event)
