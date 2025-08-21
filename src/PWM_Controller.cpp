#include "PWM_controller.hpp"

extern "C" PWM_ControllerOut::PWM_ControllerOut(const uint8_t _Pin):Pin(_Pin){}

extern "C" void PWM_ControllerOut::init(){
    PWM_Output = {
        .pin_bit_mask = (1ULL << Pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };    
    gpio_config(&PWM_Output);
}