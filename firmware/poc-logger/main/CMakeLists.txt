idf_component_register(
    SRCS "main.c" "piezo_adc.c" "lis3dh.c" "capture.c" "net.c"
    INCLUDE_DIRS "."
    REQUIRES esp_adc esp_wifi esp_netif esp_event esp_timer nvs_flash driver lwip
)
