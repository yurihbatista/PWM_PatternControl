#include <inttypes.h>
#include "hal/ledc_types.h"
#include "sdkconfig.h"
#include "PWM_controller.hpp"
#include "Wifi_connection.h"
#include "MQTT_.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "cJSON.h"

#define signalOutput_Pin 9

const char *TAG = "PWM_Control project";


controllerParameters objectParameters = {
    .Pin_0 = 9,
    .Pin_1 = 10,
    .Channel_0 = LEDC_CHANNEL_0,
    .Channel_1 = LEDC_CHANNEL_1,
    .Timer = LEDC_TIMER_0,
    .flags = {.h_bridge = 1, .AC_mode = 0}
};

PWM_Controller testObject(&objectParameters);

extern "C" void app_main()
{
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);

    if (esp_flash_get_size(NULL, &flash_size) != ESP_OK)
    {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024), (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");
    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());

    vTaskDelay(1000 / portTICK_PERIOD_MS);

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    ESP_LOGI(TAG, "Wi-Fi is up.");
    QueueHandle_t pwmQueue = testObject.init();
    mqtt_app_start(pwmQueue);
}
