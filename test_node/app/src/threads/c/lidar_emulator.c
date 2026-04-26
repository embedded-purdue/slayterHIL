#include "threads/lidar_emulator.h"
#include <zephyr/logging/log.h>
// obtain measurement sequence:
/*
 * Write DIST_WITH*_BIAS to ACQ_COMMANDS
 * Read STATUS until bit 0 (LSB) goes low
 * Read FULL_DELAY_LOW
 * Read FULL_DELAY_HIGH
 * u got ur 16 bit data measurement in centimeters!
 */

/*
 * What emulator needs to replicate:
 * Split the centimeter data to uint8_t lsb uint8_t msb
 * When ACQ_COMMANDS receive DIST_WITH*_BIAS, pull STATUS low
 * * Need interrupt
 * Send LSB to 0x10
 * Send MSB to 0x11
 * *************************************** Careful
 * IF the physics engine thread overwrites the simulated distance strictly between
 * the ESP32 reading register 0x10 and register 0x11,
 * the ESP32 reconstructs a corrupted, invalid integer.
 * so we construct a snapshot cache.
 * When the ESP32 targets register 0x10,
 * the current 16-bit distance must be frozen into this cache.
 * The subsequent sequential read targeting 0x11
 * must serve the high byte from the frozen cache,
 * strictly isolating the active I2C transfer
 * from concurrent background simulation updates.
 */
LOG_MODULE_REGISTER(lidar_emulator, LOG_LEVEL_INF);

// ISR-safe atomic state variables [cite: 497, 498]
static atomic_t simulated_distance_mm = ATOMIC_INIT(1); 
static uint16_t transaction_distance_cache = 1;

// I2C State Machine
static uint8_t active_register = 0x00;
static bool is_register_address_byte = true;

// Helper to route read requests
static uint8_t serve_register_data(uint8_t reg) {
    switch(reg) {
        case REG_STATUS:
            // Bit 0 must be 0 to indicate "Idle / Ready for command" [cite: 499]
            return 0x00; 
        case REG_FULL_DELAY_LOW:
            // Freeze the cache on the first read to prevent tearing 
            transaction_distance_cache = atomic_get(&simulated_distance_mm);
            return (uint8_t)(transaction_distance_cache & 0xFF);
        case REG_FULL_DELAY_HIGH:
            // Serve the frozen cache for the high byte
            return (uint8_t)((transaction_distance_cache >> 8) & 0xFF);
        default:
            return 0xFF; // Standard behavior for unmapped registers
    }
}

// I2C Callbacks
static int lidar_write_requested_cb(struct i2c_target_config *config) {
    // Master initiated a write. The next byte received will be the register address.
    is_register_address_byte = true;
    return 0;
}

static int lidar_write_received_cb(struct i2c_target_config *config, uint8_t val) {
    if (is_register_address_byte) {
        active_register = val;
        is_register_address_byte = false;
    } else {
        // Master is writing data TO the active_register.
        // We absorb the 0x03/0x04 acquisition commands here without doing anything, 
        // because our physics engine provides constant data[cite: 498, 499].
        // The hardware auto-increments the register pointer on sequential writes.
        active_register++;
    }
    return 0;
}

static int lidar_read_requested_cb(struct i2c_target_config *config, uint8_t *val) {
    // Master initiated a read. Serve data for the current active_register.
    *val = serve_register_data(active_register);
    return 0;
}

static int lidar_read_processed_cb(struct i2c_target_config *config, uint8_t *val) {
    // Master ACKed the previous byte and wants the next one.
    // Emulate hardware auto-increment behavior.
    active_register++;
    *val = serve_register_data(active_register);
    return 0;
}

static int lidar_stop_cb(struct i2c_target_config *config) {
    // Transaction complete. Reset state for safety.
    is_register_address_byte = true;
    return 0;
}

// Struct to bind callbacks
static struct i2c_target_callbacks lidar_callbacks = {
    .write_requested = lidar_write_requested_cb,
    .write_received = lidar_write_received_cb,
    .read_requested = lidar_read_requested_cb,
    .read_processed = lidar_read_processed_cb,
    .stop = lidar_stop_cb,
};

static struct i2c_target_config lidar_config = {
    .address = LIDAR_ADDRESS,
    .callbacks = &lidar_callbacks,
};

// Public API
void lidar_emulator_init(const struct device* i2c_dev) {
    if (!device_is_ready(i2c_dev)) {
        LOG_ERR("LiDAR I2C device not ready");
        return;
    }
    if (i2c_target_register(i2c_dev, &lidar_config) < 0) {
        LOG_ERR("Failed to register LiDAR I2C target");
    } else {
        LOG_INF("LiDAR I2C target registered at address 0x%02x", LIDAR_ADDRESS);
    }
}

void lidar_emulator_update_distance(uint16_t new_distance_mm) {
    atomic_set(&simulated_distance_mm, new_distance_mm);
}
