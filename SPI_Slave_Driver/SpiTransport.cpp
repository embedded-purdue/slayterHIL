#include "SpiTransport.hpp"
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(spi_transport, LOG_LEVEL_INF);

#define SPI_SLAVE_NODE DT_NODELABEL(spi1)

SpiTransport::SpiTransport() : 
    spi_dev(DEVICE_DT_GET(SPI_SLAVE_NODE)),
    handshake_gpio(GPIO_DT_SPEC_GET(SPI_SLAVE_NODE, handshake_gpios)) {
    
    // SPI Mode 0, 8-bit, Slave Mode
    spi_cfg.operation = SPI_WORD_SET(8) | SPI_OP_MODE_SLAVE;
    spi_cfg.frequency = 0; // External clock from Pi
    spi_cfg.slave = 0;
}

int SpiTransport::init() {
    if (!device_is_ready(spi_dev)) {
        LOG_ERR("SPI Slave device not ready");
        return -ENODEV;
    }

    if (!gpio_is_ready_dt(&handshake_gpio)) {
        LOG_ERR("Handshake GPIO not ready");
        return -ENODEV;
    }

    gpio_pin_configure_dt(&handshake_gpio, GPIO_OUTPUT_INACTIVE);
    return 0;
}

void SpiTransport::swap_buffers() {
    processed_idx = active_idx;
    active_idx = (active_idx == 0) ? 1 : 0;
}

int SpiTransport::transfer_sync() {
    struct spi_buf rx_buf = {
        .buf = buffers[active_idx],
        .len = PACKET_SIZE
    };
    const struct spi_buf_set rx_bufs = { .buffers = &rx_buf, .count = 1 };

    // Signal Pi: "I am ready and listening"
    gpio_pin_set_dt(&handshake_gpio, 1);

    // This blocks until the Pi drives the clock for exactly PACKET_SIZE bytes
    // and the NSS pin is toggled.
    int ret = spi_read(spi_dev, &spi_cfg, &rx_bufs);

    // Signal Pi: "I am busy/processing"
    gpio_pin_set_dt(&handshake_gpio, 0);

    if (ret == 0) {
        swap_buffers();
    } else {
        LOG_ERR("SPI Transfer failed: %d", ret);
    }

    return ret;
}
