
#ifndef PWM_SOUND_H
#define PWM_SOUND_H

#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdio.h>

#define PWM_CLOCK_KHZ 133000
#define AUDIO_PIN_L 26
#define AUDIO_PIN_R 27

void init_pwm();
void play_click();
void play_tone(uint freq_hz, uint duration_ms);
void play_dual_tone(uint freq1, uint freq2, uint duration_ms);
void play_dtmf_sequence(const char *digits);
void play_modem_handshake();

#endif //PWM_SOUND_H
