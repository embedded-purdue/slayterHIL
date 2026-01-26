//spi_new_interface.h
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <linux/spi/spidev.h> // Needed for SPI_MODE constants

namespace Slayter::HAL {

struct SpiConfig {
    std::string devicePath = "/dev/spidev0.0";
    uint32_t speedHz = 1000000;      // Default 1MHz
    uint8_t mode = SPI_MODE_0;       // Default Mode 0
    uint8_t bitsPerWord = 8;
    uint16_t delayUsecs = 0;
};

class SpiDriver {
public:
    // --- Constructor / Destructor ---
    // Constructor takes a config object (with defaults)
    explicit SpiDriver(const SpiConfig& config);
    
    // Destructor closes the file automatically (RAII)
    ~SpiDriver();

    // --- Safety Measures (Rule of Five) ---
    // Delete copy constructors to prevent double-closing the file descriptor
    SpiDriver(const SpiDriver&) = delete;
    SpiDriver& operator=(const SpiDriver&) = delete;

    // Allow moving (transferring ownership) - Optional but professional
    SpiDriver(SpiDriver&& other) noexcept;
    SpiDriver& operator=(SpiDriver&& other) noexcept;

    // --- Core Functionality ---
    bool open();
    void close();
    bool isOpen() const;

    // Full Duplex Transfer
    // Sends 'tx', fills 'rx' with received data. 
    // Returns true on success.
    bool transfer(const std::vector<uint8_t>& tx, std::vector<uint8_t>& rx);

    // --- Dynamic Configuration ---
    // These apply changes immediately to the hardware
    bool setSpeed(uint32_t speedHz);
    bool setMode(uint8_t mode);
    bool setBitsPerWord(uint8_t bits);

private:
    int fd_;             // File descriptor
    SpiConfig config_;   // Stores current settings

    // Helper to write current config_ to the open file descriptor
    bool applySettings(); 
};

} // namespace Slayter::HAL
