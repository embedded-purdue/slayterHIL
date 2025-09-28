#ifndef THREADS_DUT_INTERFACE_H
#define THREADS_DUT_INTERFACE_H

// Decodes protobuf messages from orchestrator. Queues actions based on timestamps and sends commands to the sensor emulation thread.

#include <stdint.h>

#define DUT_INTERFACE_STACK_SIZE (10 * 1024)
#define DUT_INTERFACE_PRIORITY 5

// definitely will need to be changed
typedef struct _DutInterfaceCommandPacket {
    uint8_t bus_id;
    uint32_t bus_data;
} _DutInterfaceCommandPacket;

#define SENSOR_UPDATE_QUEUE_PACKET_SIZE (sizeof(_DutInterfaceCommandPacket))
#define SENSOR_UPDATE_QUEUE_LEN (10)
extern struct k_msgq dut_interface_command_q;


// Init function
void dut_interface_init();

#endif /* THREADS_DUT_INTERFACE_H */