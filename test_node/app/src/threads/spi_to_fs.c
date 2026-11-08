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