#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include <inttypes.h>

// channel 3: throttle
// channel 2: for/bac
// channel 1: left/right

#define UART_NUM UART_NUM_2
#define UART_RING_BUF_SIZE 256

#define IBUS_MAX_FRAME_SIZE 32 // 32-byte frame size
#define IBUS_CHANNEL_FRAME_SIZE 32
#define IBUS_CHANNEL_CMD 0x40
#define IBUS_RX_PIN 16
#define IBUS_TIMEOUT_MS 20 // frames are sent every 7ms
#define CHANNEL_COUNT 4

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
    uart_driver_install(UART_NUM, UART_RING_BUF_SIZE, 0, 0, NULL, 0); // allocate double to avoid full buffer
}

static bool get_channel_data(uint8_t* ibus_data, uint16_t* channel_data, int channel_count)
{
    ibus_data[0] = 0;
    ibus_data[1] = 0;

    // sync to the start of a frame
    while(ibus_data[0] != IBUS_CHANNEL_FRAME_SIZE || ibus_data[1] != IBUS_CHANNEL_CMD)
    {
        int len = uart_read_bytes(UART_NUM, ibus_data, 2, IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);

        //ESP_LOGI("DEBUG", "Header bytes: %d", len);

        if (len < 1)
        {
            ESP_LOGW("CONNECTION", "Signal lost in header");
            return false;
        }
    }

    int len = uart_read_bytes(UART_NUM, &ibus_data[2], IBUS_CHANNEL_FRAME_SIZE - 2, 0);

    //ESP_LOGI("DEBUG", "Body bytes: %d", len);

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

    uint8_t ibus_data[IBUS_MAX_FRAME_SIZE];
    uint16_t channel_data[CHANNEL_COUNT];

    while (1)
    {
        if(get_channel_data(ibus_data, channel_data, CHANNEL_COUNT))
        {
            // write control logic from here on
            // channel data is saved in channel_data

            if(abs(esp_timer_get_time() % 100000) < 10000)
                ESP_LOGI("DEBUG", "%"PRIu16" %"PRIu16" %"PRIu16" %"PRIu16, channel_data[0], channel_data[1], channel_data[2], channel_data[3]);
        }

        //vTaskDelay(IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);
    }
}