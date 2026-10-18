#include "spi_interface.hpp"
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <cerrno>
#include <stdexcept>
#include <system_error>

namespace {
class SpiLinux final : public SpiInterface {
public:
    SpiLinux(const std::string& device, uint32_t speedHz) : speed(speedHz) {
        if (speed == 0) throw std::invalid_argument("SPI speed must be positive");
        fd = open(device.c_str(), O_RDWR | O_CLOEXEC);
        if (fd < 0) throw std::system_error(errno, std::generic_category(), "Open " + device);
        uint8_t mode = SPI_MODE_0;
        uint8_t bits = 8;
        uint8_t lsbFirst = 0;
        if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0 ||
            ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0 ||
            ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsbFirst) < 0 ||
            ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
            const int error = errno;
            close(fd);
            fd = -1;
            throw std::system_error(error, std::generic_category(), "Configure " + device);
        }
    }
    void transmit(const uint8_t* data, size_t len) override {
        if (!data || len != 256) throw std::invalid_argument("Expected a 256-byte frame");
        spi_ioc_transfer tr{};
        tr.tx_buf = reinterpret_cast<uintptr_t>(data);
        tr.len = static_cast<uint32_t>(len);
        tr.speed_hz = speed;
        tr.bits_per_word = 8;
        // One ioctl/transfer keeps CS asserted across the entire frame.
        const int result = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
        if (result < 0) throw std::system_error(errno, std::generic_category(), "SPI transfer");
        if (result != static_cast<int>(len)) throw std::runtime_error("Short SPI transfer");
    }
    ~SpiLinux() override { if (fd >= 0) close(fd); }
private:
    uint32_t speed;
    int fd = -1;
};
}

std::unique_ptr<SpiInterface> makeLinuxSpiTransport(const std::string& device, uint32_t speedHz) {
    return std::make_unique<SpiLinux>(device, speedHz);
}
