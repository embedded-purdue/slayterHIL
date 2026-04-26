#include <fcntl.h>              // open()
#include <unistd.h>             // close(), usleep()
#include <sys/ioctl.h>          // ioctl()
#include <linux/spi/spidev.h>   // SPI ioctl definitions
#include <cstring>              // memset()
#include <iostream>             // std::cout

int main() {
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
    return 0;
}

