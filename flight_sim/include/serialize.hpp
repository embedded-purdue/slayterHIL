#pragma once

#include <array>
#include <cstdint>
#include <imu_generation.hpp>

// Receiver contract: 2-byte LE length, protobuf payload, zero padding.
using SensorFrame = std::array<uint8_t, 256>;

// IMMEDIATE injection: timestampUs is simulation-relative metadata, not a deadline.
// IMU values are already in sensor register units; no rescaling is applied.
SensorFrame encodeSensorFrame(const imu_data_t& data, uint32_t timestampUs,
                              uint16_t lidarMm = 0,
                              int8_t rcVertical = 0, int8_t rcHorizontal = 0);
