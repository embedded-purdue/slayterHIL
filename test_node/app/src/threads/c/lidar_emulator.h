#ifndef THREADS_LIDAR_EMULATOR_H
#define THREADS_LIDAR_EMULATOR_H
#include <zephyr/kernel.h>
#include <zephyr/driver/i2c.h>

// Send 0x03: take dist. meas. without receiver bias correction
// Send 0x04: take dist. meas. with    receiver bias correction
#define ACQ_COMMANDS 0x00
#define DIST_WITH_BIAS 0x04
#define DIST_WITHOUT_BIAS 0x03
// bit 0 (lsb) goes low when acq command is completed
#define STATUS 0x01
#define FULL_DELAY_LOW 0x10
#define FULL_DELAY_HIGH 0x11

#define UNDEFINED 0xFF

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

void lidar_emulator_init(const struct device* i2c_dev);
void lidar_emulator_update_distance(uint16_t new_distance_cm);
#endif
