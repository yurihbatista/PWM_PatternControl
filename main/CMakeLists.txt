idf_component_register(
    SRCS "main.cpp" "PWM_Controller.cpp" "MQTT_.c" "Wifi_connection.c"
    INCLUDE_DIRS "."
    REQUIRES esp_driver_ledc esp_driver_gpio esp_wifi nvs_flash esp_event esp_netif mqtt cjson spi_flash esp_hw_support
)
