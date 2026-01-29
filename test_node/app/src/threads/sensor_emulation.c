#include "threads/sensor_emulation.h"
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// register logging module
LOG_MODULE_REGISTER(sensor_thread, LOG_LEVEL_INF);

K_MSGQ_DEFINE(sensor_update_q, SENSOR_UPDATE_QUEUE_PACKET_SIZE, SENSOR_UPDATE_QUEUE_LEN, 1);
K_MSGQ_DEFINE(sensor_bus_q, SENSOR_BUS_QUEUE_PACKET_SIZE, SENSOR_BUS_QUEUE_LEN, 1);

static void sensor_emulation_thread(void *, void *, void *) {
    struct k_poll_event events[2];

    k_poll_event_init(&events[0], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &sensor_update_q);
    k_poll_event_init(&events[1], K_POLL_TYPE_MSGQ_DATA_AVAILABLE, K_POLL_MODE_NOTIFY_ONLY, &sensor_bus_q);

    SensorUpdatePacket update_packet;
    SensorBusPacket bus_packet;

    while (1) {
        // dummy printing for sanity
        LOG_INF("Sensor hello world\n");

        // Get data from sensor_update_q or sensor_bus_q
        k_poll(events, 2, K_FOREVER);
        
        // Received sensor update
        if (events[0].state == K_POLL_TYPE_MSGQ_DATA_AVAILABLE) {
            k_msgq_get(&sensor_update_q, &update_packet, K_FOREVER);

            // Update sensor data

            events[0].state = K_POLL_STATE_NOT_READY;
        }

        // Received bus data
        if (events[1].state == K_POLL_TYPE_MSGQ_DATA_AVAILABLE) {
            k_msgq_get(&sensor_bus_q, &bus_packet, K_FOREVER);

            // Respond to bus message

            events[1].state = K_POLL_STATE_NOT_READY;
        }
    }
}

K_THREAD_STACK_DEFINE(sensor_emulation_stack, SENSOR_EMULATION_STACK_SIZE);
struct k_thread sensor_emulation_data;

// Initialize and start thread
void sensor_emulation_init() {
    // dummy printing for sanity
    LOG_INF("Sensor init\n");

    // Any initialization

    k_thread_create(&sensor_emulation_data, sensor_emulation_stack, 
        K_THREAD_STACK_SIZEOF(sensor_emulation_stack),
        sensor_emulation_thread,
        NULL, NULL, NULL,
        SENSOR_EMULATION_PRIORITY, 0, K_NO_WAIT);
}
