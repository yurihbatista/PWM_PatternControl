#include "PWM_controller.hpp"

extern "C" PWM_Controller::PWM_Controller(const uint8_t _Pin):Pin(_Pin){}

extern "C" void PWM_Controller::init(){
    PWM_Output = {
        .pin_bit_mask = (1ULL << Pin),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };    
    gpio_config(&PWM_Output);

    ledc_timer_config_t pwmTimer = {
        .speed_mode       = LEDC_LOW_SPEED_MODE,
        .duty_resolution  = LEDC_TIMER_10_BIT,
        .timer_num        = LEDC_TIMER_0,
        .freq_hz          = 25000,
        .clk_cfg          = LEDC_AUTO_CLK,
        .deconfigure      = false
    };
    ledc_timer_config(&pwmTimer);

    ledc_channel_config_t pwmChannel = {
        .gpio_num       = Pin,
        .speed_mode     = LEDC_LOW_SPEED_MODE,
        .channel        = LEDC_CHANNEL_0,
        .intr_type      = LEDC_INTR_DISABLE,
        .timer_sel      = LEDC_TIMER_0,
        .duty           = 0,
        .hpoint         = 0,
        .sleep_mode     = LEDC_SLEEP_MODE_KEEP_ALIVE,
        .flags          = NULL
    };
    ledc_channel_config(&pwmChannel);
}

extern "C" void PWM_Controller::control(pwmPattern* _Pattern){
    for(dualData data : _Pattern->patternSequence){
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, data.intensity);
        ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
        vTaskDelay(pdMS_TO_TICKS(data.durationMS));
    }
}