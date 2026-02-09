  //include your uart.h
  #include "uart.h"
  #include <string.h>
  #include <zephyr/drivers/uart.h>
  #include <stdalign.h> 

  void uart_callback(const struct device *dev, void *user_data)
  {
    uint8_t rx_buf[8];  
    uart_irq_update(dev);
    if(!uart_irq_rx_ready(dev))
    {
      printk("UART is not ready\n");
      return;
    } 
    while(uart_irq_rx_ready(dev))
    {
      int bytes_read = uart_fifo_read(dev, rx_buf, sizeof(rx_buf)); 
      if(bytes_read == 0){
        break;
      }
      
      struct uart_msg myMessage;
      myMessage.length = bytes_read;
      memcpy(myMessage.data,rx_buf, bytes_read);
      //below is Debug
      for(int z = 0; z<= myMessage.length; z++)
      {
        printk("%c \n",myMessage.data[z]);

    }
      
      k_msgq_put((struct k_msgq *)user_data, &myMessage, K_NO_WAIT); 
    }
  }
