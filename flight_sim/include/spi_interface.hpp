#pragma once
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

// A transport accepts exactly one complete 256-byte sensor frame per call.
// Failures throw; callers must stop rather than report a successful stream.
class SpiInterface {
public:
    virtual ~SpiInterface() = default;
    virtual void transmit(const uint8_t* data, size_t len) = 0;
};

std::unique_ptr<SpiInterface> makeCaptureTransport(const std::string& path);
std::unique_ptr<SpiInterface> makeLinuxSpiTransport(const std::string& device,
                                                   uint32_t speedHz);
