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
 * Packet model:
 *   The Pi sends ONE combined packet per tick containing IMU + LiDAR + RC.
 *   There is no sensor_id routing tag and no union — every field is always
 *   present and always valid.  The sensor thread calls all three emulator
 *   update functions on every peek.
 *
 * Queue model:
 *   sensor_update_q has exactly 1 slot.  The timer ISR is the sole writer
 *   (irq_lock → purge → put → irq_unlock).  The sensor thread is the sole
 *   reader (k_msgq_peek only — never pops).  The slot is therefore always
 *   "full" after the first injection, giving the ISR a safe target to purge
 *   at any time without an empty-queue edge case.
 */

#ifndef THREADS_SENSOR_EMULATION_H
#define THREADS_SENSOR_EMULATION_H

#include <stdint.h>
#include <zephyr/kernel.h>

/* ── Thread configuration ─────────────────────────────────────────────────── */
#define SENSOR_EMULATION_STACK_SIZE  (10U * 1024U)
/**
 * Lower number = higher priority on Zephyr.
 * Must be numerically GREATER than SCHEDULER_PRIORITY (i.e. lower priority)
 * so the scheduler thread can always preempt the sensor thread.
 */
#define SENSOR_EMULATION_PRIORITY    (7)

/* ── I2C target addresses ─────────────────────────────────────────────────── */
#define LIDAR_ADDRESS   0x62   /**< LIDAR-Lite v3 */
#define IMU_ADDRESS     0x29   /**< BNO055        */

/* ── IMU register-image types ─────────────────────────────────────────────── */

/**
 * @brief One 16-bit BNO055 field split into the LSB/MSB byte pair that maps
 *        directly onto two consecutive hardware registers.
 *
 * Example: GYR_DATA_X at registers 0x14 (LSB) and 0x15 (MSB).
 * The I2C ISR reads these bytes sequentially from the ping-pong buffer, so
 * the in-memory layout MUST match the register order.
 */
typedef struct {
    int8_t lsb;
    int8_t msb;
} i2c_imu_data_16_t;

/** @brief One axis triplet (X, Y, Z) — 6 bytes, maps to 6 consecutive regs. */
typedef struct {
    i2c_imu_data_16_t x;
    i2c_imu_data_16_t y;
    i2c_imu_data_16_t z;
} i2c_imu_triplet_t;

/**
 * @brief Complete emulated IMU register image (18 bytes).
 *
 * Field ORDER IS LOAD-BEARING.  The ping-pong buffer in imu_emulator.c is
 * read byte-by-byte starting from whichever register address the DUT writes
 * before its read, so the struct layout must match the BNO055 register map.
 *
 * BNO055 register map (subset emulated):
 *   euler_angles        0x1A – 0x1F  (6 bytes)
 *   linear_acceleration 0x28 – 0x2D  (6 bytes)  ← gap at 0x20–0x27 (quaternion)
 *   gyro                0x14 – 0x19  (6 bytes)
 *
 * @note The gap between euler (0x1F) and linear_accel (0x28) means the DUT
 *       must issue two separate I2C read transactions if it wants both groups
 *       contiguously.  Reads to unmapped registers return 0xFF.
 */
typedef struct {
    i2c_imu_triplet_t euler_angles;         /**< regs 0x1A – 0x1F */
    i2c_imu_triplet_t linear_acceleration;  /**< regs 0x28 – 0x2D */
    i2c_imu_triplet_t gyro;                 /**< regs 0x14 – 0x19 */
} imu_data_t;

_Static_assert(sizeof(imu_data_t) == 18,
               "imu_data_t must be exactly 18 bytes");

/* ── RC command type ──────────────────────────────────────────────────────── */

/**
 * @brief RC axis commands decoded from the proto RcPayload.
 *
 * Range: -127 .. +127.  Both fields = 0 means no-op (hold current state).
 * The scheduler clamps the proto sint32 values to int8 on decode.
 */
typedef struct {
    int8_t rc_vert;   /**< Vertical   axis: negative = down, positive = up   */
    int8_t rc_horiz;  /**< Horizontal axis: negative = left, positive = right */
} rc_data_t;

/* ── Combined update packet ───────────────────────────────────────────────── */

/**
 * @brief Single packet carrying all sensor state for one simulation tick.
 *
 * The Pi sends IMU + LiDAR + RC together in every SPI frame.  All fields
 * are always populated — there is no sensor_id tag and no union.
 *
 * @note timestamp_us is expressed in the STM32's own µs epoch
 *       (k_cyc_to_us_near32(k_cycle_get_32())).  The Pi must synchronise
 *       to this epoch at startup via the SPI clock-sync handshake in main.c.
 *       If the epochs are misaligned, the injection timer will never fire
 *       (packets will always appear to be in the future).
 */
typedef struct {
    uint32_t   timestamp_us;       /**< STM32 µs epoch — causality gate value   */
    imu_data_t imu_data;           /**< Full 18-byte BNO055 register image       */
    uint16_t   lidar_distance_mm;  /**< LIDAR-Lite distance in millimetres       */
    rc_data_t  rc_commands;        /**< RC axis commands (0,0 = no-op)           */
} device_update_packet_t;

/* ── 1-slot ISR gate queue ────────────────────────────────────────────────── */

/**
 * @brief The causality gate between the injection timer ISR and the sensor thread.
 *
 * Exactly ONE slot.  Protocol:
 *   Writer (timer ISR only):  irq_lock → k_msgq_purge → k_msgq_put → irq_unlock
 *   Reader (sensor thread):   k_msgq_peek — NEVER k_msgq_get
 *
 * Keeping the slot permanently full (via peek-not-pop) means the ISR can
 * always safely call purge without checking for an empty queue first.
 *
 * Alignment 4: reduces internal padding and speeds up the memcpy inside
 * Zephyr's k_msgq internals on 32-bit Cortex-M.
 */
#define SENSOR_UPDATE_QUEUE_LEN  (1U)
extern struct k_msgq sensor_update_q;

/* ── Public API ───────────────────────────────────────────────────────────── */

/**
 * @brief Initialise the sensor emulators and start the sensor emulation thread.
 *
 * Registers the I2C target callbacks for LiDAR (on i2c_lidar) and IMU
 * (on i2c_imu), then creates the sensor emulation thread.
 *
 * Must be called BEFORE scheduler_init() so that the I2C targets are
 * registered before the first injection timer fires.
 *
 * @param i2c_lidar  Zephyr device handle for the LiDAR I2C bus (i2c_lidar alias).
 * @param i2c_imu    Zephyr device handle for the IMU I2C bus   (i2c_imu alias).
 */
void sensor_emulation_init(const struct device *i2c_lidar,
                           const struct device *i2c_imu);

#endif /* THREADS_SENSOR_EMULATION_H */
