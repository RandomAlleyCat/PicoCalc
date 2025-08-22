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

    pwm_init(slice_l, &config, false);
    pwm_init(slice_r, &config, false);

    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, 0);

    pwm_set_enabled(slice_l, false);
    pwm_set_enabled(slice_r, false);
}
 
void play_click() {
    play_tone(1000, 10);
}

void play_dual_tone(uint freq1, uint freq2, uint duration_ms) {
    float clkdiv_l = (float)PWM_CLOCK_KHZ * 1000 / ((float)freq1 * wrap_value);
    float clkdiv_r = (float)PWM_CLOCK_KHZ * 1000 / ((float)freq2 * wrap_value);

    pwm_set_clkdiv(slice_l, clkdiv_l);
    pwm_set_clkdiv(slice_r, clkdiv_r);
    pwm_set_enabled(slice_l, true);
    pwm_set_enabled(slice_r, true);
    pwm_set_chan_level(slice_l, PWM_CHAN_A, wrap_value / 2);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, wrap_value / 2);
    sleep_ms(duration_ms);
    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_chan_level(slice_r, PWM_CHAN_B, 0);
    pwm_set_enabled(slice_l, false);
    pwm_set_enabled(slice_r, false);
}

void play_tone(uint freq_hz, uint duration_ms) {
    play_dual_tone(freq_hz, freq_hz, duration_ms);
}

void play_dtmf_sequence(const char *digits) {
    struct {
        char d;
        uint f1;
        uint f2;
    } tones[] = {
        {'1',697,1209}, {'2',697,1336}, {'3',697,1477}, {'A',697,1633},
        {'4',770,1209}, {'5',770,1336}, {'6',770,1477}, {'B',770,1633},
        {'7',852,1209}, {'8',852,1336}, {'9',852,1477}, {'C',852,1633},
        {'*',941,1209}, {'0',941,1336}, {'#',941,1477}, {'D',941,1633}
    };

    for (const char *p = digits; p && *p; ++p) {
        bool found = false;
        for (size_t i = 0; i < sizeof(tones)/sizeof(tones[0]); ++i) {
            if (tones[i].d == *p) {
                play_dual_tone(tones[i].f1, tones[i].f2, 60);
                sleep_ms(40);
                found = true;
                break;
            }
        }
        if (!found) {
            play_click();
        }
    }
}

void play_modem_handshake() {
    play_tone(1100, 300);
    play_tone(2100, 300);
    sleep_ms(50);
    play_tone(1300, 300);
    play_tone(2300, 300);
    sleep_ms(50);
    play_tone(1500, 400);
    play_tone(2500, 400);
}

