#ifndef PWM_CONTROLLER
#define PWM_CONTROLLER

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

class PWM_ControllerOut{
    public:
    PWM_ControllerOut(const uint8_t _Pin);
    void init();
    private:
    const uint8_t Pin;
    gpio_config_t PWM_Output;
};

#ifdef __cplusplus
}
#endif

#endif