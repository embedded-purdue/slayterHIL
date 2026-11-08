#include <stdint.h>
#include <zephyr/kernel.h>

typedef struct
{
    double v1;
    double v2;
    double v3;
    double v4;
} avg_v_packet_t;

typedef struct
{
    uint32_t timestamp_us; /**< STM32 µs epoch — causality gate value   */
    avg_v_packet_t avg_voltages;
} spi_to_fs_packet_t;

#define SPI_TO_FS_QUEUE_LEN (1U)
extern struct k_msgq spi_to_fs_q;