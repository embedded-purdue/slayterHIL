#ifndef FLIGHT_SIM_COMMS_H
#define FLIGHT_SIM_COMMS_H

#include <stdint.h>
#include <zephyr/kernel.h>

typedef struct
{
    double v[4];
} avg_v_packet_t;

typedef struct
{
    uint32_t timestamp_us; /**< STM32 µs epoch — causality gate value   */
    avg_v_packet_t avg_voltages;
} flight_sim_message_t;

#define FLIGHT_SIM_QUEUE_LEN 1U

extern struct k_msgq flight_sim_tx_q;

#define SPI_TO_FS_QUEUE_LEN (1U)
extern struct k_msgq spi_to_fs_q;