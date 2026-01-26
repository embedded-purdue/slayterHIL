#pragma once
#include <linux/spi/spidev.h>
#include <string>
#include <vector>
#include <cstdint>

class SpiInterface {
public:
    SpiDriver(const char* dev, uint8_t mode = SPI_MODE_0, uint32_t = 500000);
    ~SpiDriver();

    bool openDriver();
    bool closeDriver();



}
