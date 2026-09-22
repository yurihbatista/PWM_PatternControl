#include "PWM_controller.hpp"

PWM_Controller::PWM_Controller(const uint8_t _Pin) : Pin(_Pin) {}

QueueHandle_t PWM_Controller::init()
{
    PWM_Output = {
        .pin_bit_mask = (1ULL << Pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&PWM_Output);

    pwmTimer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = 25000,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false};
    ledc_timer_config(&pwmTimer);

    pwmChannel = {
        .gpio_num = Pin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = LEDC_CHANNEL_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,
        .flags = 0};
    ledc_channel_config(&pwmChannel);

    patternsQueue = xQueueCreate(5, sizeof(char *));
    xTaskCreatePinnedToCore(PWM_Controller::taskWrapper, "Logic task", 4096, this, 5, NULL, 0);
    xTaskCreatePinnedToCore(PWM_Controller::runWrapper, "Run task", 4096, this, 5, &RunTask, 1);
    vTaskSuspend(RunTask);

    return patternsQueue;
}

void PWM_Controller::run()
{
    for (;;)
    {
        if (pwmData == nullptr)
        {
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }

        restartRequested = false;
        ESP_LOGI("RUN_TASK", "Running Pattern: %s", pwmData->patternName.c_str());
        for (const auto &data : pwmData->patternSequence)
        {
            if (restartRequested)
                break;
            ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, data.duty);
            ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
            vTaskDelay(pdMS_TO_TICKS(data.durationMS));
            if (restartRequested)
            {
                ESP_LOGI("RUN_TASK", "Pattern Interrupted!");
                break;
            }
        }
    }
}

void PWM_Controller::taskWrapper(void *pvParameters)
{
    PWM_Controller *self = (PWM_Controller *)pvParameters;
    self->taskLogic();
}

void PWM_Controller::taskLogic()
{
    char *incoming_json = NULL;
    while (true)
    {
        if (xQueueReceive(patternsQueue, &incoming_json, portMAX_DELAY))
        {
            cJSON *root = cJSON_Parse(incoming_json);
            if (!root)
            {
                ESP_LOGE("JSON", "Invalid JSON format");
                free(incoming_json);
                continue;
            }

            cJSON *cmd = cJSON_GetObjectItem(root, "cmd");
            if (cJSON_IsString(cmd))
            {
                if (strcmp(cmd->valuestring, "STOP") == 0)
                {
                    vTaskSuspend(RunTask);
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
                }
                else if (strcmp(cmd->valuestring, "START") == 0)
                {
                    vTaskResume(RunTask);
                }
                else if (strcmp(cmd->valuestring, "SET") == 0)
                {
                    parseJsonToPattern(incoming_json);

                    restartRequested = true;
                    vTaskResume(RunTask);
                    if (RunTask != NULL)
                        xTaskAbortDelay(RunTask);
                }
            }

            cJSON_Delete(root);
            free(incoming_json);
        }
    }
}

void PWM_Controller::runWrapper(void *pvParameters)
{
    PWM_Controller *self = (PWM_Controller *)pvParameters;
    self->run();
}

void PWM_Controller::parseJsonToPattern(const char* jsonStr) {
    cJSON *root = cJSON_Parse(jsonStr);
    
    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (cJSON_IsString(name)) {
        test.patternName = name->valuestring;
    }

    cJSON *steps = cJSON_GetObjectItem(root, "steps");
    if (cJSON_IsArray(steps)) {
        test.patternSequence.clear();
        
        int arraySize = cJSON_GetArraySize(steps);
        for (int i = 0; i < arraySize; i++) {
            cJSON *item = cJSON_GetArrayItem(steps, i);
            cJSON *ms = cJSON_GetObjectItem(item, "ms");
            cJSON *duty = cJSON_GetObjectItem(item, "duty");

            if (cJSON_IsNumber(ms) && cJSON_IsNumber(duty)) {
                dualData step = { (uint16_t)ms->valueint, (uint16_t)duty->valueint };
                test.patternSequence.push_back(step);
            }
        }
    }
    
    this->pwmData = &test;
    
    cJSON_Delete(root);
}