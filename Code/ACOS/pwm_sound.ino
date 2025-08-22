#include "pwm_sound.h"
#include <math.h>
#include <stdlib.h>

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
    const uint sample_rate = 8000;
    float clkdiv = (float)PWM_CLOCK_KHZ * 1000 / ((float)sample_rate * wrap_value);
    pwm_set_clkdiv(slice_l, clkdiv);
    pwm_set_enabled(slice_l, true);

    float phase1 = 0.0f;
    float phase2 = 0.0f;
    float step1 = 2.0f * (float)M_PI * freq1 / sample_rate;
    float step2 = 2.0f * (float)M_PI * freq2 / sample_rate;
    uint total_samples = sample_rate * duration_ms / 1000;

    for (uint i = 0; i < total_samples; ++i) {
        float sample = (sinf(phase1) + sinf(phase2)) * 0.5f; // mix and scale
        uint16_t level = (uint16_t)((sample + 1.0f) * (wrap_value / 2));
        pwm_set_chan_level(slice_l, PWM_CHAN_A, level);
        sleep_us(1000000 / sample_rate);
        phase1 += step1;
        phase2 += step2;
    }

    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_l, false);
}

void play_tone(uint freq_hz, uint duration_ms) {
    play_dual_tone(freq_hz, freq_hz, duration_ms);
}

static void play_noise(uint duration_ms) {
    const uint sample_rate = 8000;
    float clkdiv = (float)PWM_CLOCK_KHZ * 1000 / ((float)sample_rate * wrap_value);
    pwm_set_clkdiv(slice_l, clkdiv);
    pwm_set_enabled(slice_l, true);

    uint total_samples = sample_rate * duration_ms / 1000;
    for (uint i = 0; i < total_samples; ++i) {
        float sample = ((float)rand() / (float)RAND_MAX) * 2.0f - 1.0f;
        uint16_t level = (uint16_t)((sample + 1.0f) * (wrap_value / 2));
        pwm_set_chan_level(slice_l, PWM_CHAN_A, level);
        sleep_us(1000000 / sample_rate);
    }

    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_l, false);
}

static void play_freq_sweep(uint start_freq, uint end_freq, uint duration_ms) {
    const uint sample_rate = 8000;
    float clkdiv = (float)PWM_CLOCK_KHZ * 1000 / ((float)sample_rate * wrap_value);
    pwm_set_clkdiv(slice_l, clkdiv);
    pwm_set_enabled(slice_l, true);

    uint total_samples = sample_rate * duration_ms / 1000;
    float phase = 0.0f;
    for (uint i = 0; i < total_samples; ++i) {
        float t = (float)i / (float)total_samples;
        float freq = start_freq + (end_freq - start_freq) * t;
        float step = 2.0f * (float)M_PI * freq / sample_rate;
        phase += step;
        float sample = sinf(phase);
        uint16_t level = (uint16_t)((sample + 1.0f) * (wrap_value / 2));
        pwm_set_chan_level(slice_l, PWM_CHAN_A, level);
        sleep_us(1000000 / sample_rate);
    }

    pwm_set_chan_level(slice_l, PWM_CHAN_A, 0);
    pwm_set_enabled(slice_l, false);
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
        bool played = false;
        for (size_t i = 0; i < sizeof(tones) / sizeof(tones[0]); ++i) {
            if (tones[i].d == *p) {
                play_dual_tone(tones[i].f1, tones[i].f2, 60);
                played = true;
                break;
            }
        }
        if (!played) {
            play_click();
        }
        sleep_ms(40);
    }
}

void play_modem_handshake() {
    play_noise(200); // line pickup noise
    play_tone(1800, 250); // brief carrier
    play_noise(80);
    play_freq_sweep(1500, 2100, 300);
    play_noise(80);
    play_freq_sweep(2100, 1200, 300);
    play_noise(100);
    play_freq_sweep(1200, 2400, 400);
    play_noise(150);
}

