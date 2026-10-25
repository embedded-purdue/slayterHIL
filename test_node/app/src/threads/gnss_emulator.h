/**
 * @file gnss_emulator.h
 * @brief GNSS emulator: UART link to the flight controller (comms only, no GPS data yet).
 */

#ifndef THREADS_GNSS_EMULATOR_H
#define THREADS_GNSS_EMULATOR_H

#include <zephyr/kernel.h>
#include <zephyr/device.h>

#define GNSS_SEND_PERIOD_MS       1000
#define GNSS_EMULATOR_STACK_SIZE  1024
#define GNSS_EMULATOR_PRIORITY    8     /* below scheduler (5) and sensor emulation (7) */

struct nmea_packet {
    char time[11];          // hhmmss.ss
    double latitude;        // Decimal degrees
    char lat_dir;           // N or S
    double longitude;       // Decimal degrees
    char lon_dir;           // E or W
    int fix_quality;        // 0 = Invalid, 1 = GPS fix, 2 = DGPS fix, etc.
    int satellites;         // Number of satellites in use. 
    double hdop;            // Horizontal dilution of precision. hardcode to 1
    double altitude;        // Altitude above mean sea level
    char alt_units;         // M (meters)

    nmea_packet(){
        
    }
}

int gnss_emulator_init(const struct device *uart_dev);
void gnss_emulator_receive()

#endif /* THREADS_GNSS_EMULATOR_H */
