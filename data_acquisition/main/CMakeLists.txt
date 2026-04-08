idf_component_register(SRCS "main.c" 
                    REQUIRES esp_wifi nvs_flash esp_event esp_netif driver esp-modbus
                    INCLUDE_DIRS "." "../../type_definitions")
