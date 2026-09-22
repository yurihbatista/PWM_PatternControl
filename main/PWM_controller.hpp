#ifndef PWM_CONTROLLER
#define PWM_CONTROLLER

#include <type_traits>
#include <cstdint>
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
#include "hal/ledc_types.h"

struct movementData
{
    uint16_t durationMS;
    uint16_t duty;
    bool dir;
};

struct pwmPattern
{
    std::string patternName;
    uint16_t patternID;
    std::vector<movementData> patternSequence;
};

struct controllerParameters
{
    uint8_t Pin_0;
    uint8_t Pin_1;
    ledc_channel_t Channel_0;
    ledc_channel_t Channel_1;
    ledc_timer_t Timer;
    struct controllerParameters_flags {
        bool h_bridge: 1;
        bool AC_mode: 1;
    } flags;
};


class PWM_Controller
{
public:
    PWM_Controller(const controllerParameters *parameters);
    QueueHandle_t init();

private:
    void sweepResonance();
    QueueHandle_t patternsQueue;
    static void taskWrapper(void *pvParameters);
    void taskLogic();
    ledc_timer_config_t pwmTimer;
    ledc_channel_config_t pwmChannel_0;  
    ledc_channel_config_t pwmChannel_1;
    controllerParameters c_parameters;
    gpio_config_t PWM_Output;
    void run();
    static void runWrapper(void* pvParameters);
    TaskHandle_t RunTask;
    pwmPattern* pwmData = nullptr;
    volatile bool restartRequested = false;
    void parseJsonToPattern(const char* jsonStr);
    pwmPattern test{
    "Test",
    0,
    {{1000, 1024, 0}, {1000, 0, 0}}};
};

#endif
