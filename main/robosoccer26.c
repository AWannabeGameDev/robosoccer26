#if 1

#include "driver/uart.h"
#include "esp_log.h"
#include "esp_timer.h"
#include <math.h>
#include <inttypes.h>

#define UART_NUM UART_NUM_2
#define UART_RING_BUF_SIZE 256
#define IBUS_MAX_FRAME_SIZE 32 // 32-byte frame size
#define IBUS_CHANNEL_FRAME_SIZE 32
#define IBUS_RX_PIN 16
#define IBUS_TIMEOUT_MS 8 // frames are sent every 7ms
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
    while(ibus_data[0] != IBUS_CHANNEL_FRAME_SIZE || ibus_data[1] != 0x40)
    {
        int len = uart_read_bytes(UART_NUM, ibus_data, 2, IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);

        //ESP_LOGI("DEBUG", "Header bytes: %d", len);

        if (len < 1)
        {
            ESP_LOGW("CONNECTION", "Signal lost in header");
            return false;
        }
    }

    int len = uart_read_bytes(UART_NUM, &ibus_data[2], IBUS_CHANNEL_FRAME_SIZE - 2, IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);

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

        vTaskDelay(20 / portTICK_PERIOD_MS);
    }
}

#else

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/mcpwm_cap.h"
#include "esp_log.h"

#define PWM_INPUT_GPIO 18 // Connect FS-iA6B Ch1 here

static const char *TAG = "RC_PWM";

// This callback runs in ISR context - keep it lean
static bool IRAM_ATTR on_capture_callback(mcpwm_cap_channel_handle_t cap_chan, const mcpwm_capture_event_data_t *edata, void *user_data) {
    static uint32_t last_edge = 0;
    uint32_t *pulse_width_us = (uint32_t *)user_data;

    if (edata->cap_edge == MCPWM_CAP_EDGE_POS) {
        last_edge = edata->cap_value;
    } else {
        // Delta between pos and neg edges
        *pulse_width_us = edata->cap_value - last_edge;
    }
    return false; 
}

void app_main(void) {
    uint32_t pulse_width = 0;

    // 1. Setup Capture Timer
    mcpwm_capture_timer_config_t timer_conf = {
        .group_id = 0,
        .clk_src = MCPWM_CAPTURE_CLK_SRC_DEFAULT,
    };
    mcpwm_cap_timer_handle_t cap_timer = NULL;
    mcpwm_new_capture_timer(&timer_conf, &cap_timer);

    // 2. Setup Capture Channel
    mcpwm_capture_channel_config_t chan_conf = {
        .gpio_num = PWM_INPUT_GPIO,
        .prescale = 1, 
        .flags.neg_edge = true,
        .flags.pos_edge = true,
        .flags.pull_up = true, // RC signals are usually active high, internal pull-up helps
    };
    mcpwm_cap_channel_handle_t cap_chan = NULL;
    mcpwm_new_capture_channel(cap_timer, &chan_conf, &cap_chan);

    // 3. Register Callback
    mcpwm_capture_event_callbacks_t cb = {
        .on_cap = on_capture_callback,
    };
    mcpwm_capture_channel_register_event_callbacks(cap_chan, &cb, &pulse_width);

    // 4. Enable and Start
    mcpwm_capture_channel_enable(cap_chan);
    mcpwm_capture_timer_enable(cap_timer);
    mcpwm_capture_timer_start(cap_timer);

    while (1) {
        // pulse_width is being updated in the background via ISR
        ESP_LOGI(TAG, "Ch1 Width: %ld us", pulse_width);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

#endif