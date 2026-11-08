#include "threads/spi_to_fs.h"

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

K_MSGQ_DEFINE(spi_to_fs_q,
              sizeof(spi_to_fs_packet_t),
              SPI_TO_FS_QUEUE_LEN,
              4);

// set up spi overlay

// init

// create thread
K_THREAD_DEFINE(tn_to_fs, STACK_SIZE, 
    my_entry_point, arg1, arg2, arg3, 
    PRIORITY, 0, 0);
