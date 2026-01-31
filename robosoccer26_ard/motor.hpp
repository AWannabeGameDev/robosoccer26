#ifndef MOTOR_H
#define MOTOR_H

#include <Arduino.h>

void motor_init(void);
void motor_set(int16_t left, int16_t right);

#endif