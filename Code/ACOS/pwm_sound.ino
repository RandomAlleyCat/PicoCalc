#include "pwm_sound.h"

static uint slice_l;
static uint slice_r;
static const uint16_t wrap_value = 1000;

void init_pwm() {
    gpio_set_function(AUDIO_PIN_L, GPIO_FUNC_PWM);
    gpio_set_function(AUDIO_PIN_R, GPIO_FUNC_PWM);

    slice_l = pwm_gpio_to_slice_num(AUDIO_PIN_L);
    slice_r = pwm_gpio_to_slice_num(AUDIO_PIN_R);

    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, 125.0f);
    pwm_config_set_wrap(&config, wrap_value);

    pwm_init(slice_l, &config, true);
    pwm_init(slice_r, &config, true);

    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, 0);
}

void play_click() {
    pwm_set_chan_level(slice_l, PWM_CHAN_A, wrap_value / 2);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, wrap_value / 2);
    sleep_ms(10);
    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, 0);
}

