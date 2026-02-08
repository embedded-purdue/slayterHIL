  //include your uart.h
  #include "uart.h"
  #include <string.h>
  #include <zephyr/drivers/uart.h>
  #include <stdalign.h> //try to remove alignof because it is older c 


  //intiatie a callback to some state machine 

  /*

  uart_irq_callback_user_data_set ->uart_irq_rx_enable -> make this can define void uart_cb(const struct device *dev, void *user_data) -> uart_irq_update(dev) -> uart_irq_rx_ready(dev) -> uart_irq_rx_ready(dev) - > k_msgq_put(&my_msgq, &msg, K_NO_WAIT)


  main():
    ├─ DEVICE_DT_GET
    ├─ device_is_ready
    ├─ uart_irq_callback_user_data_set
    └─ uart_irq_rx_enable

  UART interrupt occurs
    └─ uart_cb(dev, user_data)
        ├─ uart_irq_update
        ├─ uart_irq_rx_ready
        ├─ uart_fifo_read (loop)
        └─ k_msgq_put (K_NO_WAIT)

  worker thread:
    └─ k_msgq_get

  ISR fires
  → update irq state
  → while RX ready:
        read fifo chunk
        push chunk to msgq

        what buffer size pass to fifo read ? -> this is how Test_node will interact with us 
  */

  void uart_callback(const struct device *dev, void *user_data)
  {
    printk("callback trigger\n");
    uint8_t rx_buf[8]; //this is problem i need a variable 
    uart_irq_update(dev);
    if(!uart_irq_rx_ready(dev))
    {
      printk("Uart is not ready\n"); //-> find another way to print or call out there is an error 
      return;
    } //this is also unneccesaryt 
    while(uart_irq_rx_ready(dev))
    {
      //defensive 
      //when the fifo_read returns a 0 then we break out of the loo p
      int bytes_read = uart_fifo_read(dev, rx_buf, sizeof(rx_buf)); //get all the messages in the message buffer 
        printk("%d\n",bytes_read);
      if(bytes_read == 0){
        printk("byte 0\n");
        break;
      }
      
      struct uart_msg myMessage;
    
      myMessage.length = bytes_read;

      memcpy(myMessage.data,rx_buf, bytes_read);
      for(int z = 0; z<= myMessage.length; z++)
      {
        printk("%c \n",myMessage.data[z]);

    }
      
      //is the fifo_read not directly going into my rx_buf ?
      k_msgq_put((struct k_msgq *)user_data, &myMessage, K_NO_WAIT); //this is not done but i need to define the message quee 
      /*
      K_MSGQ_DEFINE(uart_rx_msgq,
                sizeof(struct uart_msg),
                QUEUE_LEN,
                alignof(struct uart_msg)); when i define
      in ym message queue i use the get funciton which is k_msgq_get with all my data0*/
    }
  }
