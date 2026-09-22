#include "PWM_controller.hpp"
#include "cJSON.h"
#include "hal/ledc_types.h"

PWM_Controller::PWM_Controller(const controllerParameters *parameters) : c_parameters(*parameters) {}

QueueHandle_t PWM_Controller::init()
{
    uint64_t pin_mask = (1ULL << c_parameters.Pin_0);
    if (c_parameters.flags.h_bridge == 1) {
        pin_mask |= (1ULL << c_parameters.Pin_1);
    }

    PWM_Output = {
        .pin_bit_mask = pin_mask,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE};
    gpio_config(&PWM_Output);

    pwmTimer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_10_BIT,
        .timer_num = c_parameters.Timer,
        .freq_hz = 100,
        .clk_cfg = LEDC_AUTO_CLK,
        .deconfigure = false};
    ledc_timer_config(&pwmTimer);

    pwmChannel_0 = {
        .gpio_num = c_parameters.Pin_0,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = c_parameters.Channel_0,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0,
        .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,
        .flags = { .output_invert = 0 },
        .deconfigure = false};
    ledc_channel_config(&pwmChannel_0);

    if (c_parameters.flags.h_bridge == 1) {
        int phase = 0;
        if(c_parameters.flags.AC_mode == 1) 
        {
            phase = 512;
        } else {
            phase = 0;
        }

        pwmChannel_1 = {
            .gpio_num = c_parameters.Pin_1,
            .speed_mode = LEDC_LOW_SPEED_MODE,
            .channel = c_parameters.Channel_1,
            .intr_type = LEDC_INTR_DISABLE,
            .timer_sel = c_parameters.Timer,
            .duty = 0,
            .hpoint = phase,
            .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,
            .flags = { .output_invert = 0 },
            .deconfigure = false};
        ledc_channel_config(&pwmChannel_1);
    }

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
            if(c_parameters.flags.h_bridge == 1 && c_parameters.flags.AC_mode == 0){
               if(data.dir == 0){
                   ESP_LOGI("RUN_TASK", "Direction 0");
                   ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1, 0);
                   ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1);
                   ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0, data.duty);
                   ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0);
               } else if(data.dir == 1) {
                   ESP_LOGI("RUN_TASK", "Direction 1");
                   ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0, 0);
                   ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0);
                   ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1, data.duty);
                   ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1);
               } else {
                   ESP_LOGE("RUN_TASK", "Unhandled!");
               }
            } else if (c_parameters.flags.h_bridge == 1 && c_parameters.flags.AC_mode == 1) {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0, data.duty);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0);
                ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1, data.duty);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1);
            } else if (c_parameters.flags.h_bridge == 0) {
                ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0, data.duty);
                ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0);
            } else {
                ESP_LOGE("RUN_TASK", "Unhandled");
            }
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
                    ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0, 0);
                    ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0);
                    
                    if (c_parameters.flags.h_bridge == 1) {
                        ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1, 0);
                        ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1);
                    }
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
                else if (strcmp(cmd->valuestring, "SWEEP") == 0)
                {
                    sweepResonance();
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
            cJSON *dir = cJSON_GetObjectItem(item, "dir");

            if (cJSON_IsNumber(ms) && cJSON_IsNumber(duty)) {
                bool direction = cJSON_IsNumber(dir) ? (dir->valueint != 0) : false;
                movementData step = { (uint16_t)ms->valueint, (uint16_t)duty->valueint, direction };
                test.patternSequence.push_back(step);
            }
        }
    }
    
    this->pwmData = &test;
    
    cJSON_Delete(root);
}

void PWM_Controller::sweepResonance() {
    ESP_LOGI("SWEEP", "Starting fine LRA frequency sweep (150Hz - 250Hz)...");
    vTaskSuspend(RunTask);
    
    ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0, 512);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0);
    
    if (c_parameters.flags.h_bridge == 1) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1, 512);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1);
    }

    for (uint32_t freq = 115; freq <= 125; freq += 1) {
        ESP_LOGI("SWEEP", "Testing Frequency: %" PRIu32 " Hz", freq);
        
        ledc_set_freq(LEDC_LOW_SPEED_MODE, c_parameters.Timer, freq);
        
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_0);
    
    if (c_parameters.flags.h_bridge == 1) {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1, 0);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, c_parameters.Channel_1);
    }
    ESP_LOGI("SWEEP", "Sweep complete.");
}
