/**
 * @file sensor_emulation.h
 * @brief Shared types and queue declarations for the HIL sensor emulation layer.
 *
 * Architecture overview:
 *   Scheduler Thread  ──(scheduler_q, N-slot ring)──►  Timer ISR
 *   Timer ISR         ──(sensor_update_q, 1-slot gate)──► Sensor Thread
 *   Sensor Thread     ──(direct fn calls)──► Emulator update fns
 *   I2C ISRs          ──(atomic / ping-pong read)──► DUT
 *
 * The 1-slot sensor_update_q is NEVER popped by the sensor thread (k_msgq_peek
 * only). The timer ISR owns writes via an irq_lock/purge/put sequence, giving
 * it deterministic overwrite semantics with zero queue-clog risk.
 */

#ifndef THREADS_SENSOR_EMULATION_H
#define THREADS_SENSOR_EMULATION_H

#include <stdint.h>
#include <zephyr/kernel.h>

/* ── Thread configuration ─────────────────────────────────────────────────── */
#define SENSOR_EMULATION_STACK_SIZE  (4096U)
/** Lower number = higher priority. Must be lower priority than scheduler. */
#define SENSOR_EMULATION_PRIORITY    (7)

/* ── I2C target addresses ─────────────────────────────────────────────────── */
#define LIDAR_ADDRESS   0x62
#define IMU_ADDRESS     0x29

/* ── Sensor device IDs (payload routing tag) ──────────────────────────────── */
#define LIDAR_DEVICE_ID  (0x01U)
#define IMU_DEVICE_ID    (0x02U)
#define RC_DEVICE_ID     (0x03U)

/* ── IMU register-image types ─────────────────────────────────────────────── */

/**
 * @brief A single 16-bit IMU field split into the LSB/MSB byte pair that maps
 *        directly onto consecutive BNO055 hardware registers.
 */
typedef struct {
    int8_t lsb;
    int8_t msb;
} i2c_imu_data_16_t;

/** @brief One axis triplet (x, y, z) from the BNO055. */
typedef struct {
    i2c_imu_data_16_t x;
    i2c_imu_data_16_t y;
    i2c_imu_data_16_t z;
} i2c_imu_triplet_t;

/**
 * @brief Complete emulated IMU register image.
 *
 * Field order matches the ping-pong buffer layout used by imu_emulator.c.
 * Each triplet is 6 bytes; total = 18 bytes.
 *
 * BNO055 register map (subset):
 *   gyro            0x14 – 0x19  (6 bytes)
 *   euler_angles    0x1A – 0x1F  (6 bytes)
 *   linear_accel    0x28 – 0x2D  (6 bytes)
 */
typedef struct {
    i2c_imu_triplet_t gyro;               /**< regs 0x14–0x19 */
    i2c_imu_triplet_t euler_angles;       /**< regs 0x1A–0x1F */
    i2c_imu_triplet_t linear_acceleration;/**< regs 0x28–0x2D */
} imu_data_t;

_Static_assert(sizeof(imu_data_t) == 18,
               "imu_data_t must be exactly 18 bytes");

/* ── Update packet ────────────────────────────────────────────────────────── */

/**
 * @brief Tagged-union update packet flowing from scheduler → sensor thread.
 *
 * @note  timestamp_us uses the STM32's own hardware clock epoch
 *        (k_cyc_to_us_near32). The Pi MUST have been synchronised to this
 *        epoch at startup via the SPI handshake protocol, otherwise the
 *        temporal gating calculation in the timer ISR is meaningless.
 */
typedef struct {
    /** STM32 local µs at which this physical state becomes causally valid. */
    uint32_t timestamp_us;
    /** Routes the union payload to the correct emulator. */
    uint8_t  sensor_id;
    union {
        imu_data_t imu_data;
        uint16_t   lidar_distance_mm;
        uint8_t    rc_command;
    };
} device_update_packet_t;

/* ── 1-slot ISR gate queue ────────────────────────────────────────────────── */

/**
 * Exactly ONE slot.  The timer ISR writes via irq_lock/purge/put.
 * The sensor thread reads via k_msgq_peek (never pops).
 * This keeps the slot perpetually "full" so the ISR can always purge safely.
 */
#define SENSOR_UPDATE_QUEUE_LEN  (1U)
extern struct k_msgq sensor_update_q;

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Initialise and start the sensor emulation thread.
 *
 * @param i2c_lidar  Zephyr device handle for the LiDAR I2C bus.
 * @param i2c_imu    Zephyr device handle for the IMU I2C bus.
 */
void sensor_emulation_init(const struct device *i2c_lidar,
                           const struct device *i2c_imu);

#endif /* THREADS_SENSOR_EMULATION_H */
