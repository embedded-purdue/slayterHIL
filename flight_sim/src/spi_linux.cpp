#include "spi_interface.h"
#include <fcntl.h>
#include <unistd.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <iostream>

class SpiLinux : public SpiInterface {
public:
    SpiLinux(const char* dev = "/dev/spidev0.0",
             uint8_t mode = SPI_MODE_0,
             uint32_t speed = 500000,
             uint8_t bits = 8)
        : devPath(dev), mode(mode), speed(speed), bits(bits), fd(-1) {}

    bool init() override {
        fd = open(devPath, O_RDWR);
        if (fd < 0) {
            std::cerr << "Failed to open SPI device: " << devPath << "\n";
            return false;
        }
        if (ioctl(fd, SPI_IOC_WR_MODE, &mode) == -1 ||
            ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) == -1 ||
            ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) == -1) {
            std::cerr << "Failed to configure SPI device.\n";
            return false;
        }
        return true;
    }

    bool transmit(const uint8_t* data, size_t len) override {
        struct spi_ioc_transfer tr{};
        tr.tx_buf = (unsigned long)data;
        tr.len = len;
        tr.speed_hz = speed;
        tr.bits_per_word = bits;
        int ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 1) {
            std::cerr << "SPI transmit failed.\n";
            return false;
        }
        return true;
    }

    ~SpiLinux() {
        if (fd >= 0) close(fd);
    }

private:
    const char* devPath;
    uint8_t mode;
    uint32_t speed;
    uint8_t bits;
    int fd;
};
