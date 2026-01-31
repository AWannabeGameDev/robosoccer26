#include <Arduino.h>

#include "motor.h"

#define IBUS_CHANNEL_FRAME_SIZE 32
#define IBUS_CHANNEL_CMD 0x40
#define IBUS_TIMEOUT_MS 20
#define CHANNEL_COUNT 4
#define JOYSTICK_DEADZONE 20

void setup() 
{
    // iBus usually runs at 115200
    Serial.begin(115200);
    Serial.setTimeout(IBUS_TIMEOUT_MS);

    motor_init();

    // Use Serial1 (GPIO 2 / D4) for debugging logs if needed
    // Serial1.begin(115200);
}

bool get_channel_data(uint16_t* channel_data, int channel_count) 
{
    static uint8_t ibus_data[32];

    int len = Serial.readBytes(ibus_data, 2);

    if(len < 2)
    {
        Serial.println("Failed to read header");
        return false;
    }

    if(ibus_data[0] != IBUS_CHANNEL_FRAME_SIZE || ibus_data[1] != IBUS_CHANNEL_CMD) return false;

    len = Serial.readBytes(&ibus_data[2], 30);

    if(len < 30)
    {
        Serial.println("Failed to read body");
        return false;
    }

    uint16_t calc_checksum = 0xFFFF - ibus_data[0] - ibus_data[1];

    for (int i = 2; i <= 2 * CHANNEL_COUNT + 1; i += 2) 
    {
        calc_checksum -= ibus_data[i] + ibus_data[i + 1];
        channel_data[(i - 2) / 2] = ibus_data[i] | (ibus_data[i + 1] << 8);
    }

    // Rest of checksum calculation
    for (int i = 2 * CHANNEL_COUNT + 2; i <= 29; i++) 
    {
        calc_checksum -= ibus_data[i];
    }

    uint16_t actual_checksum = ibus_data[30] | (ibus_data[31] << 8);

    return (calc_checksum == actual_checksum);
}

void loop() 
{
    static uint16_t channel_data[CHANNEL_COUNT];

    if(get_channel_data(channel_data, CHANNEL_COUNT)) 
    {
        int16_t speed = channel_data[2] - 1000;
        int16_t for_bac = 0;

        if (channel_data[1] > (1500 + JOYSTICK_DEADZONE)) 
        {
            for_bac = 1;
        } 
        else if (channel_data[1] < (1500 - JOYSTICK_DEADZONE)) 
        {
            for_bac = -1;
        }

        motor_set(speed * for_bac, speed * for_bac);
    }
}