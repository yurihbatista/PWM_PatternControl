#include <stdio.h>
#include <string>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "PWM_controller.hpp"

#define OnOff_Pin 21
#define switch_Pin 22

#define signalOutput_Pin 23

const uint64_t Buttons_Mask = (1ULL << OnOff_Pin) | (1ULL << switch_Pin);

gpio_config_t Buttons_Input = {
    .pin_bit_mask = Buttons_Mask,
    .mode = GPIO_MODE_INPUT,
    .pull_up_en = GPIO_PULLUP_ENABLE,
    .pull_down_en = GPIO_PULLDOWN_DISABLE,
    .intr_type = GPIO_INTR_NEGEDGE
};

PWM_Controller OutputTest(signalOutput_Pin);

pwmPattern test {
    "Test",
    0,
    {{1000,1024},{1000,0}}
};

extern "C" void app_main() {
    printf("Initializing GPIOs\n\n");
    gpio_config(&Buttons_Input);
    OutputTest.init();
    gpio_dump_io_configuration(stdout, (1ULL << signalOutput_Pin)|(Buttons_Mask));

    while(true){
        OutputTest.control(&test);
    }
}