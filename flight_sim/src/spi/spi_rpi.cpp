//spi_rpi.cpp
#pragma once
#include "slayter/hal/spi_new_interface.h" //CMake allows to reference from root of the include folder
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <iostream>
#include <cstring>

namespace slayter::HAL {
    // Constructor, constructs a SpiDriver obj with given config
    // and sets file descriptor to -1 (invalid)
    // body empty cus no op
    SpiDriver::SpiDriver(const SpiConfig& config):
        fd_(-1), config_(config) {}
    // Destructor, closes the fd to ensure no leak
    SpiDriver::~SpiDriver() {
        close();
    }
    
    //Move constructor: take ownership from 'other'
    SpiDriver::SpiDriver(SpiDriver&& other) noexcept:
        fd_(other.fd), config_(other.config_) {
            other.fd_ = -1 //invalidate original so it doesn't close on destruction
        }
    
    //Move assignment
    SpiDriver& SpiDriver::operator=(SpiDriver&& other) noexcept {
        if (this != &other) {
            close(); //closes 'this' resources
            fd_ = other.fd_;
            config_ = other.config_;
            other.fd_ = -1;
        }
        return *this;
    }
    bool SpiDriver::open() {
        if (isOpen()) {
            return true; //already open
        }
        fd_ = ::open(config_.device_path.c_str(), O_RDWR);
        if (fd_ < 0) {
            std::cerr << "[SPI] Error: Couldn't open " << config_.device_path << "\n";
            return false;
        }
        //apply initial configuartion
        if (!applySettings()) {
            close();
            return false;
        }
        return true;
    }
    void SpiDriver::close() {
        if (fd_ > 0) {
            ::close(fd_);
            fd_ = -1;
        }
    }
    bool SpiDriver::isOpen() {
        return fd_ >= 0;
    }
    bool SpiDriver::applySettings() {
        if (!isOpen()) {
            return false;
        }
        int ret = 0;
        // configure the kernel(?), flags are from spidev.h
        ret |= ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &config_.speedHz);
        ret |= ioctl(fd_, SPI_IOC_WR_MODE, &config_.mode);
        ret |= ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &config_.bitsPerWord);
        if (ret < 0) {
            std::cerr << "[SPI] Error: Failed to apply SPI Settings.\n";
            return false;
        } 
        return true;
    }
    bool SpiDriver::setSpeed(uint32_t speedHz) {
        config_.speedHz = speedHz;
        // directly ioctl or applySettings()
        int ret |= ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, speedHz);
        if (ret < 0) {
            std::cerr << "[SPI] Error: Failed to set speed \n";
            return false;
        }
        return true;
    }
    bool SpiDriver::setMode(uint8_t mode) {
        config_.mode = mode;
        //directly ioctl or applysettings()
        int ret = ioctl(fd_, SPI_IOC_WR_MODE, mode);
        if (ret < 0) {
            std::cerr << "[SPI] Error: Failed to set mode \n";
            return false;
        }
        return true;
    }
    bool SpiDriver::setBitsPerWord(uint8_t bitsPerWord) {
        config_.bitsPerWord = bitsPerWord;
        int ret = ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, bitsPerWord);
        if (ret < 0) {
            std::cerr << "[SPI] Failed to set BPW\n";
            return false;
        }
        return true;
    }
    // use std::vector for dynamic szing
    //to send, configure the tx correctly, rx don't matter if u don't need
    //to receive, empty tx and transmit
    bool SpiDriver::transmit(std::vector<uint8_t>& tx, std::vector<uint8_t>& rx) {
        if (!isOpen()) {
            std::cerr << "[SPI] Error: Transfer attempted on closed driver\n";
            return false;
        }
        if (tx.empty()) return true;
        //Full duplex: size must match
        if (rx.size() != tx.size()) {
            rx.resize(tx.size());
        }
        struct spi_ioc_transfer tr;
        std::memset(&tr, 0, sizeof(tr)); // 0 out the mem first
        tr.tx_buf = (unsigned long) tx.data();
        tr.rx_buf = (unsigned long) rx.data();
        tr.len = tx.size(); // tx size = rx size
        tr.speed_hz = config_.speedHz;
        tr.bits_per_word = config_.bitsPerWord;
        tr.delay_usecs = config_.delayUsecs;
        tr.cs_change = false; //true to deselect device before starting the next transfer

        // send 1 (one, uno, ein) message
        // at the same time receive to rx_buf
        // as it is full duplex ^
        int ret = ioctl(fd_, SPI_IOC_MESSAGE(1), &tr);
        if (ret < 0) {
            std::cerr << "[SPI] Error: Transfer ioctl failed.\n";
            return false;
        }
        return true;
    }
}
