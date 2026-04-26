#ifndef SPI_TRANSPORT_HPP
#define SPI_TRANSPORT_HPP

#include <zephyr/kernel.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>

class SpiTransport {
public:
    static constexpr size_t PACKET_SIZE = 256;

    SpiTransport();
    int init();

    /**
     * @brief Pulls handshake high and waits for the Pi to send 256 bytes.
     * @return 0 on success, negative error code on failure.
     */
    int transfer_sync();

    /**
     * @brief Returns a pointer to the buffer containing the most recent data.
     */
    uint8_t* get_latest_data() { return buffers[processed_idx]; }

private:
    const struct device *spi_dev;
    struct spi_config spi_cfg;
    struct gpio_dt_spec handshake_gpio;

    uint8_t buffers[2][PACKET_SIZE];
    uint8_t active_idx = 0;    // Buffer currently being filled by SPI
    uint8_t processed_idx = 1; // Buffer available for the app to read
    
    void swap_buffers();
};

#endif
