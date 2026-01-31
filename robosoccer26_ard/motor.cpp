#include "motor.h"

/* ===== PIN DEFINITIONS ===== */
// Note: Use GPIO numbers, not Pin D-numbers (e.g., D1 is GPIO 5)
// Ensure these pins support PWM on ESP8266
#define LEFT_RPWM 15 // D8
#define LEFT_LPWM 13 // D7
#define LEFT_REN  12 // D6
#define LEFT_LEN  14 // D5

#define RIGHT_RPWM 2  // D4
#define RIGHT_LPWM 0  // D3
#define RIGHT_REN  4  // D2
#define RIGHT_LEN  5  // D1

#define PWM_DEADZONE 15
#define PWM_MAX 1023

void motor_init() 
{
    pinMode(LEFT_RPWM, OUTPUT);
    pinMode(LEFT_LPWM, OUTPUT);
    pinMode(LEFT_REN, OUTPUT);
    pinMode(LEFT_LEN, OUTPUT);
    
    pinMode(RIGHT_RPWM, OUTPUT);
    pinMode(RIGHT_LPWM, OUTPUT);
    pinMode(RIGHT_REN, OUTPUT);
    pinMode(RIGHT_LEN, OUTPUT);

    // Enable the drivers
    digitalWrite(LEFT_REN, HIGH);
    digitalWrite(LEFT_LEN, HIGH);
    digitalWrite(RIGHT_REN, HIGH);
    digitalWrite(RIGHT_LEN, HIGH);

    // Set PWM frequency to 20kHz to match your ESP-IDF code
    analogWriteFreq(20000);
    analogWriteRange(PWM_MAX);
}

void motor_set(int16_t left, int16_t right) 
{
    // Clamping
    left = constrain(left, -PWM_MAX, PWM_MAX);
    right = constrain(right, -PWM_MAX, PWM_MAX);

    // LEFT SIDE
    if (left > PWM_DEADZONE) 
    {
        analogWrite(LEFT_RPWM, left);
        analogWrite(LEFT_LPWM, 0);
    } 
    else if (left < -PWM_DEADZONE) 
    {
        analogWrite(LEFT_RPWM, 0);
        analogWrite(LEFT_LPWM, -left);
    } 
    else 
    {
        analogWrite(LEFT_RPWM, 0);
        analogWrite(LEFT_LPWM, 0);
    }

    // RIGHT SIDE
    if (right > PWM_DEADZONE) 
    {
        analogWrite(RIGHT_RPWM, right);
        analogWrite(RIGHT_LPWM, 0);
    } 
    else if (right < -PWM_DEADZONE) 
    {
        analogWrite(RIGHT_RPWM, 0);
        analogWrite(RIGHT_LPWM, -right);
    } 
    else 
    {
        analogWrite(RIGHT_RPWM, 0);
        analogWrite(RIGHT_LPWM, 0);
    }
}