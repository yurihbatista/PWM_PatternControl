#include "MQTT_.h"

static QueueHandle_t target_queue = NULL;
void process_mqtt_json(const char *data, int len)
{
    char *json_str = (char *)malloc(len + 1);
    memcpy(json_str, data, len);
    json_str[len] = '\0';

    cJSON *root = cJSON_Parse(json_str);
    if (root)
    {
        cJSON *pwm_val = cJSON_GetObjectItem(root, "pwm");
        if (cJSON_IsNumber(pwm_val))
        {
            int val = pwm_val->valueint;
            ESP_LOGI(TAG, "Received PWM value: %d", val);
        }
        cJSON_Delete(root);
    }
    else
    {
        ESP_LOGE(TAG, "JSON Parse Error");
    }
    free(json_str);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT Connected to HiveMQ!");
        esp_mqtt_client_subscribe(event->client, MQTT_TOPIC, 1);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT Disconnected");
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT Data Received");
        if (target_queue != NULL)
        {
            char *json_payload = (char *)malloc(event->data_len + 1);
            if (json_payload)
            {
                memcpy(json_payload, event->data, event->data_len);
                json_payload[event->data_len] = '\0';
                if (xQueueSend(target_queue, &json_payload, 0) != pdTRUE)
                {
                    ESP_LOGE(TAG, "Queue full, dropping JSON");
                    free(json_payload);
                }
            }
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT Error. Type: %d", event->error_handle->error_type);
        break;
    default:
        break;
    }
}

void mqtt_app_start(QueueHandle_t data_queue)
{
    target_queue = data_queue;
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = MQTT_URL;
    mqtt_cfg.broker.verification.crt_bundle_attach = esp_crt_bundle_attach;
    mqtt_cfg.credentials.username = MQTT_USER;
    mqtt_cfg.credentials.authentication.password = MQTT_PASS;

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}