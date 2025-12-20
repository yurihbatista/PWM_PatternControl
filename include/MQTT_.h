#ifndef MQTT_
#define MQTT_

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "Auth_data.h"

#ifdef __cplusplus
extern "C"
{
#endif

    extern const char *TAG;
    void mqtt_app_start(QueueHandle_t data_queue);

#ifdef __cplusplus
}
#endif

#endif