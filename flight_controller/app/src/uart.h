#ifndef UART_H
#define UART_H

#include <zephyr/kernel.h>
#include <stdint.h>

struct uart_msg{
    uint8_t data[8];
    uint8_t length;
};

extern struct k_msgq uart_rx_msgq;  

void uart_callback(const struct device *dev, void *user_data);
int uart_init(const struct device *dev); 

#endif