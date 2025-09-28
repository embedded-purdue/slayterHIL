#ifndef THREADS_SENSOR_EMULATION_H
#define THREADS_SENSOR_EMULATION_H

// Decodes protobuf messages from orchestrator. Queues actions based on timestamps and sends commands to the sensor emulation thread.

#define SENSOR_EMULATION_STACK_SIZE (10 * 1024)
#define SENSOR_EMULATION_PRIORITY 5

// definitely will need to be changed
typedef struct _SensorUpdatePacket {
    uint8_t sensor_id;
    uint32_t sensor_data;
} SensorUpdatePacket;

// definitely will need to be changed
typedef struct _SensorBusPacket {
    uint8_t bus_id;
    uint32_t bus_data;
} SensorBusPacket;

#define SENSOR_UPDATE_QUEUE_PACKET_SIZE (sizeof(SensorUpdatePacket))
#define SENSOR_UPDATE_QUEUE_LEN (10)
extern struct k_msgq sensor_update_q;

#define SENSOR_BUS_QUEUE_PACKET_SIZE (sizeof(SensorBusPacket))
#define SENSOR_BUS_QUEUE_LEN (10)
extern struct k_msgq sensor_bus_q;

// Init function
void sensor_emulation_init();

#endif /* THREADS_SENSOR_EMULATION_H */