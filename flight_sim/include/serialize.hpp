#include <vector>
#include "imu_generation.hpp"

// Class to serialize data for SPI data sending
// Data inputted from imu_generation.cpp
class Serialize {
    public:
        Serialize();
        ~Serialize();

        // Method to serialize data
        uint8_t* serialize(const imu_data_t& data, uint8_t *buffer);

};