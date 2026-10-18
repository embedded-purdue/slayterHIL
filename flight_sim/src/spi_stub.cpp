#include "spi_interface.hpp"
#include <fstream>
#include <stdexcept>

namespace {
class CaptureTransport final : public SpiInterface {
public:
    explicit CaptureTransport(const std::string& path)
        : output(path, std::ios::binary | std::ios::trunc) {
        if (!output) throw std::runtime_error("Cannot open capture: " + path);
    }
    void transmit(const uint8_t* data, size_t len) override {
        if (!data || len != 256) throw std::invalid_argument("Expected a 256-byte frame");
        output.write(reinterpret_cast<const char*>(data), len);
        output.flush();
        if (!output) throw std::runtime_error("Failed to write capture frame");
    }
private:
    std::ofstream output;
};
}

std::unique_ptr<SpiInterface> makeCaptureTransport(const std::string& path) {
    return std::make_unique<CaptureTransport>(path);
}

#ifndef __linux__
std::unique_ptr<SpiInterface> makeLinuxSpiTransport(const std::string&, uint32_t) {
    throw std::runtime_error("SPI transport requires Linux spidev; use --transport capture on this host");
}
#endif
