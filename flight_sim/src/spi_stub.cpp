#include "spi_interface.h"
#include <iostream>
#include <fstream>
#include <iomanip>

//this is a stub implementation of the SPI interface for testing without actual hardware
//switch to spi_linux.cpp after obtaining properly configured hardware (rpi)

class SpiStub : public SpiInterface {
public:
    bool init() override {
        std::cout << "[SPI Stub] Initialized.\n";
        return true;
    }

    bool transmit(const uint8_t* data, size_t len) override {
        std::ofstream f("spi_payload.log", std::ios::app | std::ios::binary);
        f.write(reinterpret_cast<const char*>(data), len);
        f.close();

        std::cout << "[SPI Stub] Wrote " << len << " bytes to spi_payload.log\n";
        // Print hex for inspection
        for (size_t i = 0; i < len; i++) {
            std::cout << std::hex << std::setw(2) << std::setfill('0')
                      << static_cast<int>(data[i]) << " ";
        }
        std::cout << std::dec << "\n";
        return true;
    }
};
