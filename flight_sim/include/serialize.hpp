#pragma once

#include <stdint.h>
#include <imu_generation.hpp>

// Method to serialize data
extern uint8_t* serialize(const imu_data_t& data, uint8_t *buffer);
