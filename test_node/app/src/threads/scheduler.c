#include "threads/scheduler.h"
#include "threads/orchestrator_comms.h" // for orchestrator_received_q
#include "threads/sensor_emulation.h" // for sensor data update queue
#include <sensor_data.pb.h> // protobuf struct definition header
#include <pb_decode.h> // protobuf decode header
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

// register logging module
LOG_MODULE_REGISTER(scheduler_thread, LOG_LEVEL_INF);


static void scheduler_thread(void *, void *, void *) {
    OrchestratorCommsPacket packet;
    SensorPacket sensorPack;
    int statusForDecode;
    device_update_packet_t lidarPacket;
    device_update_packet_t rcPacket;
    device_update_packet_t imuPacket;
    while(1) {
        // dummy printing for sanity
        LOG_INF("Scheduler hello world\n");

        memset(&lidarPacket, 0, sizeof(device_update_packet_t)); // 0 out mem
        memset(&rcPacket, 0, sizeof(device_update_packet_t)); // 0 out mem
        memset(&imuPacket, 0, sizeof(device_update_packet_t)); // 0 out mem
        
        // Get data from orchestrator_receive_q
        k_msgq_get(&orchestrator_receive_q, &packet, K_FOREVER);

        // Decode using protobuf
        sensorPack = SensorPacket_init_zero;
        pb_istream_t stream = pb_istream_from_buffer(packet.data, packet.len);
        statusForDecode = pb_decode(&stream, SensorPacket_fields, &sensorPack);
        if (!statusForDecode) {
            LOG_ERR("Failed to decode Protobuf in scheduler_thread");
        }

        //LiDAR, type = uint16_t
        //for now, let's assume LiDAR distance measurement is stored as .lidar_data_mm
        lidarPacket.sensor_id = SENSOR_ID_LIDAR;
        lidarPacket.lidar_distance_mm = sensorPack.lidar_data_mm; 

        //RC, type = char[]
        //for now, let's assume RC command is stored as .rc_data
        rcPacket.sensor_id = SENSOR_ID_RC;
        memcpy(&rcPacket.rc_command, sensorPack.rc_data, sizeof(sensorPack.rc_data)); 

        //IMU, type = struct of quat + trip + trip + trip of 2*uint8_t
        //right now, the imu_data_t in sensor_emulator.h does NOT match the protobuf schema
        //so again, let's assume IMU is stored as .imu_data
        imuPacket.sensor_id = SENSOR_ID_IMU;
        memcpy(&imuPacket.imu_data, sensorPack.imu_data, sizeof(sensorPack.imu_data));

        // Schedule sensor data updates (using a heap?)
        /*
         * I do not think heap would be the move here bcs of dynamic memalloc and memory fragmentation
         * maybe priority queue with constant size? idk
         *  - kaan
         */
        // At appropriate times, put sensor data updates into sensor data update queue (may need interrupts)
    }
}

K_THREAD_STACK_DEFINE(scheduler_stack, SCHEDULER_STACK_SIZE);
struct k_thread scheduler_data;

// Initialize and start thread
void scheduler_init() {
    // dummy printing for sanity
    LOG_INF("Scheduler init\n");

    // Any initialization

    k_thread_create(&scheduler_data, scheduler_stack, 
        K_THREAD_STACK_SIZEOF(scheduler_stack),
        scheduler_thread,
        NULL, NULL, NULL,
        SCHEDULER_PRIORITY, 0, K_NO_WAIT);
}
