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

static void play_tone_pair(uint f1, uint f2, uint duration_ms) {
    uint wrap1 = PWM_CLOCK_KHZ * 1000 / f1;
    uint wrap2 = PWM_CLOCK_KHZ * 1000 / f2;
    pwm_set_wrap(slice_l, wrap1);
    pwm_set_wrap(slice_r, wrap2);
    pwm_set_chan_level(slice_l, PWM_CHAN_A, wrap1 / 2);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, wrap2 / 2);
    sleep_ms(duration_ms);
    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, 0);
    pwm_set_wrap(slice_l, wrap_value);
    pwm_set_wrap(slice_r, wrap_value);
}

void play_dtmf_sequence(char digit) {
    struct {
        char d;
        uint f1;
        uint f2;
    } tones[] = {
        {'1',697,1209}, {'2',697,1336}, {'3',697,1477},
        {'4',770,1209}, {'5',770,1336}, {'6',770,1477},
        {'7',852,1209}, {'8',852,1336}, {'9',852,1477},
        {'0',941,1336}
    };
    for (size_t i = 0; i < sizeof(tones)/sizeof(tones[0]); ++i) {
        if (tones[i].d == digit) {
            play_tone_pair(tones[i].f1, tones[i].f2, 100);
            return;
        }
    }
    play_click();
}

void play_modem_handshake() {
    play_tone_pair(1100, 2100, 300);
    sleep_ms(50);
    play_tone_pair(1300, 2300, 300);
    sleep_ms(50);
    play_tone_pair(1500, 2500, 400);
}

