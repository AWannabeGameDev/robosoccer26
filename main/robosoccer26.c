#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

// Define your 4 GPIOs here
const int CH_PINS[] = {18, 19, 21, 22};
volatile uint32_t pulse_widths[4] = {0};
volatile uint64_t start_times[4] = {0};

// ISR Handler - IRAM_ATTR keeps this in RAM for speed
static void IRAM_ATTR gpio_isr_handler(void* arg)
{
    int ch = (int)arg;
    uint64_t now = esp_timer_get_time();

    if(gpio_get_level(CH_PINS[ch]))
    {
        // Rising edge
        start_times[ch] = now;
    }
    else
    {
        // Falling edge
        uint32_t diff = (uint32_t)(now - start_times[ch]);
        if (diff > 900 && diff < 2100)
        { // Sanity check for RC signals
            pulse_widths[ch] = diff;
        }
    }
}

static void init_gpio()
{
    gpio_install_isr_service(0);

    for (int i = 0; i < 4; i++)
    {
        gpio_config_t io_conf = 
        {
            .intr_type = GPIO_INTR_ANYEDGE, // Trigger on both up and down
            .mode = GPIO_MODE_INPUT,
            .pin_bit_mask = (1ULL << CH_PINS[i]),
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_ENABLE,
        };

        gpio_config(&io_conf);
        gpio_isr_handler_add(CH_PINS[i], gpio_isr_handler, (void*)i);
    }
}

void app_main(void)
{
    init_gpio();

    while (1)
    {
        printf("CH1: %4ld | CH2: %4ld | CH3: %4ld | CH4: %4ld\n",
               pulse_widths[0], pulse_widths[1], pulse_widths[2], pulse_widths[3]);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}