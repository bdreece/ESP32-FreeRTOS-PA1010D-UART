#include <stdio.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define U1TXD   GPIO_NUM_10
#define U1RXD   GPIO_NUM_9

xQueueHandle Global_Queue_Handle = 0;

void receive_nmea(void *param)
{
    const int uart_num = *(const int *)param;
    printf("%d\n", uart_num);

    while (1)
    {
        uint8_t data[128];
        int length = 0;
        ESP_ERROR_CHECK(uart_get_buffered_data_len(uart_num, (size_t *)&length));
        length = uart_read_bytes(uart_num, data, length, 100);

        if (!xQueueSend(Global_Queue_Handle, &data, 1000))
        {
            printf("Failed to send NMEA data to the queue!\n");
        }
    }

    vTaskDelete(NULL);
}

void print_nmea(void *param)
{
    uint8_t data[128];
    while (1)
    {
        if (xQueueReceive(Global_Queue_Handle, &data, 1000))
        {
            printf("%s\n", (char *)data);
        } else {
            printf("Failed to receive NMEA data from the queue!");
        }
    }
}

void app_main(void)
{
    int uart_num = UART_NUM_1;
    uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_CTS_RTS,
        .rx_flow_ctrl_thresh = 122
    };

    ESP_ERROR_CHECK(uart_param_config((const int)uart_num, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin((const int)uart_num, U1TXD, U1RXD, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    const int uart_buffer_size = (1024 * 2);
    QueueHandle_t uart_queue;

    ESP_ERROR_CHECK(uart_driver_install((const int)uart_num, uart_buffer_size, uart_buffer_size, 10, &uart_queue, 0));

    char *pmtk_output_rmc = "$PMTK314,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0*29";
    char *pmtk_update_1Hz = "$PMTK220,1000*1F";

    uart_write_bytes((const int)uart_num, (const char *)pmtk_output_rmc, strlen(pmtk_output_rmc));
    uart_write_bytes((const int)uart_num, (const char*)pmtk_update_1Hz, strlen(pmtk_update_1Hz));

    Global_Queue_Handle = xQueueCreate(10, 128 * sizeof(uint8_t));

    xTaskCreate(receive_nmea, "receiveNMEA", 2048, &uart_num, 1, NULL);
    xTaskCreate(print_nmea, "printNMEA", 2048, NULL, 1, NULL);
}
