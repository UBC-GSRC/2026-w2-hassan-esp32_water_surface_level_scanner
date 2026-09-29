idf_component_register(
    SRCS "test_espnow_link.c"
    INCLUDE_DIRS 
        "."
        "../../../../../type_definitions"
    REQUIRES unity espnow_link esp_wifi
)