#include <driver/gpio.h>
#include <driver/ledc.h>
#include <esp_log.h>

#include <stdlib.h>

#include "motor.h"

/* ===== PIN DEFINITIONS ===== */

// LEFT BTS7960
#define LEFT_RPWM 26
#define LEFT_LPWM 27
#define LEFT_REN 33
#define LEFT_LEN 32

// RIGHT BTS7960
#define RIGHT_RPWM 17
#define RIGHT_LPWM 18
#define RIGHT_REN 19
#define RIGHT_LEN 21

/* ===== PWM CONFIG ===== */

#define PWM_FREQ 20000
#define PWM_RES LEDC_TIMER_10_BIT
#define PWM_MAX 1023
#define PWM_DEADZONE 15

/* ===== INIT ===== */

void motor_init(void)
{
    // Enable pins
    gpio_set_direction(LEFT_REN, GPIO_MODE_OUTPUT);
    gpio_set_direction(LEFT_LEN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RIGHT_REN, GPIO_MODE_OUTPUT);
    gpio_set_direction(RIGHT_LEN, GPIO_MODE_OUTPUT);

    gpio_set_level(LEFT_REN, 1);
    gpio_set_level(LEFT_LEN, 1);
    gpio_set_level(RIGHT_REN, 1);
    gpio_set_level(RIGHT_LEN, 1);

    // PWM timer
    ledc_timer_config_t timer = 
    {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ,
        .duty_resolution = PWM_RES,
        .clk_cfg = LEDC_AUTO_CLK
    };

    ledc_timer_config(&timer);

    // PWM channels
    ledc_channel_config_t ch = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0};

    ch.channel = LEDC_CHANNEL_0;
    ch.gpio_num = LEFT_RPWM;
    ledc_channel_config(&ch);

    ch.channel = LEDC_CHANNEL_1;
    ch.gpio_num = LEFT_LPWM;
    ledc_channel_config(&ch);

    ch.channel = LEDC_CHANNEL_2;
    ch.gpio_num = RIGHT_RPWM;
    ledc_channel_config(&ch);

    ch.channel = LEDC_CHANNEL_3;
    ch.gpio_num = RIGHT_LPWM;
    ledc_channel_config(&ch);
}

/* ===== MAIN CONTROL ===== */

// values are clamped between -1023 and 1023
void motor_set(int16_t left, int16_t right)
{
    if (left < -PWM_MAX) left = -PWM_MAX;
    else if (left > PWM_MAX) left = PWM_MAX;
    if (right < -PWM_MAX) right = -PWM_MAX;
    else if (right > PWM_MAX) right = PWM_MAX;

    //ESP_LOGI("PWM", "left : %d",(int)left);

    // LEFT SIDE
    if (left > PWM_DEADZONE)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, left);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);

    }
    else if (left < -PWM_DEADZONE)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, -left);
    }
    else
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1, 0);
          
    }

    //ESP_LOGI("PWM", "right : %d",(int)right);

    // RIGHT SIDE
    if (right > PWM_DEADZONE)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, right);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0);
    }
    else if (right < -PWM_DEADZONE)
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, -right);
    }
    else
    {
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2, 0);
        ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3, 0);
    }

    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_1);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_2);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_3);
}
