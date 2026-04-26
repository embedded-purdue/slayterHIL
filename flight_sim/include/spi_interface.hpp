#pragma once
#include <cstddef>
#include <cstdint>

// Pure virtual interface for SPI (bridge)
class SpiInterface {
public:
    virtual ~SpiInterface() = default;
    virtual bool init() = 0;
    virtual bool transmit(const uint8_t* data, size_t len) = 0;
};
