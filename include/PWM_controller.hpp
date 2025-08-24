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

typedef struct {
    uint16_t durationMS;
    uint16_t intensity;
} dualData;

typedef struct {
    std::string patternName;
    uint16_t patternID;
    std::vector<dualData> patternSequence;
} pwmPattern;

#ifdef __cplusplus
extern "C" {
#endif

class PWM_Controller{
    public:
    PWM_Controller(const uint8_t _Pin);
    void init();
    void control(pwmPattern* _Pattern);

    private:
    ledc_timer_config_t pwmTimer;
    ledc_channel_config_t pwmChannel;
    const uint8_t Pin;
    gpio_config_t PWM_Output;
};

#ifdef __cplusplus
}
#endif

#endif