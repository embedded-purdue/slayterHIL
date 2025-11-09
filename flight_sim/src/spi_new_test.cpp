#include <fcntl.h>              // open()
#include <unistd.h>             // close(), usleep()
#include <sys/ioctl.h>          // ioctl()
#include <linux/spi/spidev.h>   // SPI ioctl definitions
#include <cstring>              // memset()
#include <iostream>             // std::cout

#include "../../shared/proto/sensor_data.pb.h"

#define SPI_MODE 	(SPI_MODE_0) 
#define SPI_BITS 	(8)
#define SPI_SPEED 	(100000)

void serialize_to_proto (char *msg) {
// Meters per second
    float acceleration_x = 0.01f;
    float acceleration_y = 0.02f;
    float acceleration_z = -9.81f;

    // Meters per second^2
    float angular_velocity_x = 0.03f;
    float angular_velocity_y = 0.04f;
    float angular_velocity_z = 0.05f;

    // In Palscals
    float pressure = 123000.11f;

    // Meters
    float altitude = 100.15f;

    // In Celcius
    float temperature = 22.5f;

    // Timestamp in milliseconds from beginning
    unsigned int timestamp = 1000;

    auto packet = std::make_unique<SensorPacket>();
    
    packet->set_timestamp(timestamp);
    
    // Populate nested data
    ImuData* imu = packet->mutable_imu_data();
    Vector3D* accel = imu->mutable_acceleration();
    accel->set_x(acceleration_x);
    accel->set_y(acceleration_y);
    accel->set_z(acceleration_z);

    Vector3D* gyro = imu->mutable_angular_velocity();
    gyro->set_x(angular_velocity_x);
    gyro->set_y(angular_velocity_y);
    gyro->set_z(angular_velocity_z);

    AltimeterData* altimeter = packet->mutable_altimeter_data();
    altimeter->set_pressure(pressure);
    altimeter->set_altitude(altitude);
    altimeter->set_temperature(temperature);

    printf("Acceleration x,y,z: %f, %f, %f\n", acceleration_x, acceleration_y, acceleration_z);
    printf("Angular Velocity x,y,z: %f, %f, %f\n", angular_velocity_x, angular_velocity_y, angular_velocity_z);
    printf("Pressure: %f, Altitude: %f, Temperature: %f\n", pressure, altitude, temperature);
    

    // Serialize data into string to send over spi
    std::string serialized_data;
    if (!packet->SerializeToString(&serialized_data)) {
        std::cerr << "Failed to serialize packet." << std::endl;
        return -1;
    }
}

/*
 * msg is protobuf
 */

void send_spi(char *msg, size_t size, int fd, int bits, int speed, int mode) {
    // ---- 3. Prepare data ----
    uint8_t tx_buf[] = { 'H', 'E', 'L', 'L', 'O' };  // outbound data
    uint8_t rx_buf[sizeof(tx_buf)] = {0};                         // inbound buffer
    size_t length = sizeof(tx_buf);                               // total bytes

    // ---- 4. Build transaction ----
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx_buf;     // pointer to transmit buffer
    tr.rx_buf = (unsigned long)rx_buf;     // pointer to receive buffer
    tr.len = length;                       // bytes to transfer
    tr.speed_hz = speed;                   // override per-transfer speed
    tr.bits_per_word = bits;               // bits per word
    tr.delay_usecs = 10;                   // 10 µs delay after each CS toggle

    // ---- 6. Execute SPI transaction ----
    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("ioctl");
        close(fd);
        return 1;
    }

    // ---- 7. Wait briefly before reading output ----

    // ---- 8. Print received data ----
    std::cout << "RX bytes (" << length << "):";
    for (size_t i = 0; i < length; ++i)
        std::cout << " 0x" << std::hex << (int)rx_buf[i];
    std::cout << std::dec << std::endl;

    // ---- 9. Cleanup ----
    close(fd);
}

/*
 * fd_out is a reference to an external variable to pass the file descriptor
 */

void init_spi(int *fd_out, uint8_t *mode_out, uint8_t *bits_out, uint32_t *speed_out) {
    // ---- 1. Device path ----
    const char* dev = "/dev/spidev0.0";  // SPI0 bus, CS0 pin
    int fd = open(dev, O_RDWR);
    if (fd < 0) { perror("open"); return 1; }

    // ---- 2. Basic configuration ----
    uint8_t mode = SPI_MODE_0;           // CPOL = 0, CPHA = 0 (must match ESP32)
    uint8_t bits = 8;                    // 8 bits per word
    uint32_t speed = 100000;             // 100 kHz start (slow for stability)

    // Apply configuration using ioctl()
    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)  perror("mode");
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) perror("bits");
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) perror("speed");

    *fd_out = fd;
    *mode_out = mode;
    *bits_out = bits;
    *speed_out = speed;
}

