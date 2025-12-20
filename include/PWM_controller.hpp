#ifndef PWM_CONTROLLER
#define PWM_CONTROLLER

#include <stdio.h>
#include <string>
#include <stdlib.h>
#include <vector>
#include "driver/ledc.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "cJSON.h"

typedef struct
{
    uint16_t durationMS;
    uint16_t duty;
} dualData;

typedef struct
{
    std::string patternName;
    uint16_t patternID;
    std::vector<dualData> patternSequence;
} pwmPattern;

class PWM_Controller
{
public:
    PWM_Controller(const uint8_t _Pin);
    QueueHandle_t init();

private:
    QueueHandle_t patternsQueue;
    static void taskWrapper(void *pvParameters);
    void taskLogic();
    ledc_timer_config_t pwmTimer;
    ledc_channel_config_t pwmChannel;
    const uint8_t Pin;
    gpio_config_t PWM_Output;
    void run();
    static void runWrapper(void* pvParameters);
    TaskHandle_t RunTask;
    pwmPattern test{
    "Test",
    0,
    {{1000, 1024}, {1000, 0}}};
    pwmPattern* pwmData = nullptr;
    volatile bool restartRequested = false;
    void parseJsonToPattern(const char* jsonStr);
};

#endif