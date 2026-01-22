#include "driver/uart.h"
#include "esp_log.h"

#define UART_NUM UART_NUM_1
#define IBUS_BUF_SIZE 32 // 32-byte frame size
#define IBUS_RX_PIN 16
#define IBUS_TIMEOUT_MS 8 // frames are sent every 7ms

void app_main(void)
{
    uart_config_t uart_config = 
    {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };

    uart_param_config(UART_NUM, &uart_config);
    uart_set_pin(UART_NUM, UART_PIN_NO_CHANGE, IBUS_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM, IBUS_BUF_SIZE * 2, 0, 0, NULL, 0); // allocate double to avoid full buffer

    uint8_t channel_data[IBUS_BUF_SIZE];

    while (1)
    {
        int len = uart_read_bytes(UART_NUM, channel_data, IBUS_BUF_SIZE - 1, IBUS_TIMEOUT_MS / portTICK_PERIOD_MS);

        if (len > 0)
        {
            channel_data[len] = '\0'; // Null-terminate for printing
            printf("Recv: %s\n", (char*)channel_data);
        }

        // You don't actually need a vTaskDelay here because
        // uart_read_bytes blocks and lets the CPU do other things anyway.
    }
}