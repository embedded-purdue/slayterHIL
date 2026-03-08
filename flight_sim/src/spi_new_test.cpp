#include <fcntl.h>              // open()
#include <unistd.h>             // close(), usleep()
#include <sys/ioctl.h>          // ioctl()
#include <linux/spi/spidev.h>   // SPI ioctl definitions
#include <cstring>              // memset(), memcpy()
#include <iostream>             // std::cout, std::cerr
#include <memory>               // std::make_unique
#include <cstdlib>              // malloc(), free()

// NEW proto (generated from node_imu.proto)
#include "../../shared/proto/node_imu.pb.h"

#define SPI_MODE 	(SPI_MODE_0)
#define SPI_BITS 	(8)
#define SPI_SPEED 	(100000)

void serialize_to_proto(char **msg, size_t *length) {
    // Example values (set what you actually want)
    uint32_t timestamp_ms = 1000;
    uint32_t lidar_cm     = 123;
    uint32_t command      = 0;

    auto packet = std::make_unique<DeviceUpdatePacket>();

    packet->set_timestamp(timestamp_ms);
    packet->set_lidar(lidar_cm);
    packet->set_command(command);

    // If you don't need IMU yet, leave it default (still a valid message).
    // If you DO want IMU, uncomment and fill fields once you confirm ImuData16 layout.
    /*
    ImuPayload* imu = packet->mutable_imu();

    // Example of touching nested messages without setting ImuData16 fields:
    imu->mutable_quaternion();
    imu->mutable_linear_acceleration();
    imu->mutable_gyro();
    */

    // Serialize data into string to send over spi
    std::string serialized_data;
    if (!packet->SerializeToString(&serialized_data)) {
        std::cerr << "Failed to serialize DeviceUpdatePacket." << std::endl;
        *msg = nullptr;
        *length = 0;
        return;
    }

    size_t length_msg = serialized_data.size();
    *msg = (char*)malloc(length_msg);
    if (!*msg) {
        std::cerr << "Failed to allocate message buffer." << std::endl;
        *length = 0;
        return;
    }

    memcpy(*msg, serialized_data.data(), length_msg);
    *length = length_msg;

    std::cout << "Serialized DeviceUpdatePacket bytes: " << *length << std::endl;
}

/*
 * msg is protobuf
 */
void send_spi(char *msg, size_t size, int fd, int bits, int speed, int mode) {
    // Prepare data
    uint8_t *tx_buf = (uint8_t*)msg;  // outbound data
    std::cout << "TX size: " << size << std::endl;

    // NOTE: protobuf is binary; don't print as %s.
    if (size >= 5) {
        std::cout << "TX first 5 bytes (dec): "
                  << (int)tx_buf[0] << ","
                  << (int)tx_buf[1] << ","
                  << (int)tx_buf[2] << ","
                  << (int)tx_buf[3] << ","
                  << (int)tx_buf[4] << std::endl;
    }

    uint8_t rx_buf[size];
    memset(rx_buf, 0, sizeof(rx_buf));
    size_t length = size;

    // Build transaction
    struct spi_ioc_transfer tr;
    memset(&tr, 0, sizeof(tr));
    tr.tx_buf = (unsigned long)tx_buf;
    tr.rx_buf = (unsigned long)rx_buf;
    tr.len = length;
    tr.speed_hz = speed;
    tr.bits_per_word = bits;
    tr.delay_usecs = 10;

    // Execute SPI transaction
    int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
    if (ret < 1) {
        perror("ioctl");
        close(fd);
        return;
    }

    // Print received data
    std::cout << "RX bytes (" << length << "):";
    for (size_t i = 0; i < length; ++i)
        std::cout << " 0x" << std::hex << (int)rx_buf[i];
    std::cout << std::dec << std::endl;

    // Cleanup
    close(fd);
}

/*
 * fd_out is a reference to an external variable to pass the file descriptor
 */
void init_spi(int *fd_out, uint8_t *mode_out, uint8_t *bits_out, uint32_t *speed_out) {
    const char* dev = "/dev/spidev0.0";  // SPI0 bus, CS0 pin
    int fd = open(dev, O_RDWR);
    if (fd < 0) { perror("open"); return; }

    uint8_t mode = SPI_MODE_0;
    uint8_t bits = 8;
    uint32_t speed = 50000;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0)  perror("mode");
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) perror("bits");
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) perror("speed");

    *fd_out = fd;
    *mode_out = mode;
    *bits_out = bits;
    *speed_out = speed;
}

int main() {
    int fd_out = 0;
    size_t length = 0;
    uint8_t mode_out = 0;
    uint8_t bits_out = 0;
    uint32_t speed_out = 0;

    init_spi(&fd_out, &mode_out, &bits_out, &speed_out);

    char *msg = nullptr;
    serialize_to_proto(&msg, &length);
    if (!msg || length == 0) {
        std::cerr << "No message to send." << std::endl;
        if (fd_out > 0) close(fd_out);
        return 1;
    }

    // IMPORTANT: send_spi signature is (msg, size, fd, bits, speed, mode)
    send_spi(msg, length, fd_out, bits_out, speed_out, mode_out);

    free(msg);
    return 0;
}
