#include <zephyr/kernel.h>
#include <stdint.h>

struct uart_msg{
    uint8_t data[8];//array
    uint8_t length;
    // this is going to be sthe sizse of the mssage 
};
// what the uart message will be 


extern struct k_msgq uart_rx_msgq;  //where we will send the messages to 

void uart_callback(const struct device *dev, void *user_data);