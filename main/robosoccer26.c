#include <driver/uart.h>
#include <driver/gpio.h>
#include <esp_log.h>

#include <math.h>
#include <inttypes.h>

#include "motor.h"

// channel 2: throttle
// channel 1: for/bac
// channel 0: left/right

#define UART_NUM UART_NUM_2
#define UART_RING_BUF_SIZE 256
#define UART_BUFFER_FULL_THRES 8

#define IBUS_MAX_FRAME_SIZE 32 // 32-byte frame size
#define IBUS_CHANNEL_FRAME_SIZE 32
#define IBUS_CHANNEL_CMD 0x40
#define IBUS_RX_PIN 16
#define IBUS_TIMEOUT_MS 20 // frames are sent every 7ms
#define CHANNEL_COUNT 4
#define JOYSTICK_DEADZONE 20

static void setup_ibus()
{
    uart_config_t uart_config = 
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT
    };

    uart_param_config(UART_NUM, &uart_config);

    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, IBUS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    gpio_set_pull_mode(IBUS_RX_PIN, GPIO_PULLUP_ONLY); // to reduce electrical noise

    // uart_intr_config_t rx_intr_config =
    // {
    //     .rxfifo_full_thresh = UART_BUFFER_FULL_THRES,
    //      .rx_timeout_thresh = IBUS_TIMEOUT_MS
    // };

    // uart_intr_config(UART_NUM, &rx_intr_config);
    // uart_enable_rx_intr(UART_NUM);

    uart_driver_install(UART_NUM, UART_RING_BUF_SIZE, 0, 0, NULL, 0);
}

static bool get_channel_data(uint8_t* ibus_data, uint16_t* channel_data, int channel_count)
{
    ibus_data[0] = 0;
    ibus_data[1] = 0;

    // sync to the start of a frame
    while(ibus_data[0] != IBUS_CHANNEL_FRAME_SIZE || ibus_data[1] != IBUS_CHANNEL_CMD)
    {
        int len = uart_read_bytes(UART_NUM, ibus_data, 2, IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);

        if(len < 0)
        {
            ESP_LOGE("ERROR", "Failed to read header bytes");
            return false;
        }

        if (len < 1)
        {
            ESP_LOGW("CONNECTION", "Signal lost in header");
            return false;
        }
    }

    int len = uart_read_bytes(UART_NUM, &ibus_data[2], IBUS_CHANNEL_FRAME_SIZE - 2, IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);

    if(len < 0)
    {
        ESP_LOGE("ERROR", "Failed to read body bytes");
        return false;
    }

    if(len < IBUS_CHANNEL_FRAME_SIZE - 2)
    {
        ESP_LOGW("CONNECTION", "Signal lost in body");
        return false;
    }

    uint16_t calc_checksum = 0xFFFF - ibus_data[0] - ibus_data[1];

    for(int i = 2; i <= 2 * channel_count + 1; i += 2) // read two bytes at a time now to get channels data
    {
        calc_checksum -= ibus_data[i] + ibus_data[i + 1];
        channel_data[(i - 2) / 2] = ibus_data[i] | (ibus_data[i + 1] << 8); // little-endian
    }

    for(int i = 2 * channel_count + 2; i <= 29; i++)
    {
        calc_checksum -= ibus_data[i];
    }

    uint16_t actual_checksum = ibus_data[30] | (ibus_data[31] << 8);

    if(calc_checksum != actual_checksum)
    {
        return false;
    }

    return true;
}

void app_main(void)
{
    setup_ibus();
    motor_init();

    uint8_t ibus_data[IBUS_MAX_FRAME_SIZE];
    uint16_t channel_data[CHANNEL_COUNT];

    while (1)
    {
        if(get_channel_data(ibus_data, channel_data, CHANNEL_COUNT))
        {
            int16_t speed = channel_data[2] - 1000;
            int16_t for_bac;
            int16_t steer = channel_data[0]-1500;
            ESP_LOGI("data","%d",channel_data[1]);

            if(channel_data[1] > (1500 + JOYSTICK_DEADZONE))
            {
                for_bac = 1;
            }
            else if(channel_data[1] < (1500 - JOYSTICK_DEADZONE))
            {
                for_bac = -1;
            }
            else
            {
                for_bac = 0;
            }

            motor_set(speed * for_bac+steer/10, speed * for_bac-steer/10);
        }

        //vTaskDelay(IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);
    }
}