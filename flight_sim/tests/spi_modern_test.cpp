#include "slayter/hal/spi_new_interface.h" // Your SPI Driver
#include "sensor_data.pb.h"               // Your Generated Protobuf
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <cmath> // For sin/cos simulation

// --- Configuration ---
const std::string SPI_DEVICE = "/dev/spidev0.0";
const uint32_t SPI_SPEED = 1000000; // 1MHz
const int SEND_RATE_HZ = 100;       // 100 times per second
const int TOTAL_PACKETS = 1000;     // How many to send before stopping (0 = infinite)

// --- Helper: Populate Protobuf with Dummy Physics Data ---
void update_physics_model(SensorPacket& packet, int step_count) {
    // 1. Timestamp (milliseconds)
    packet.set_timestamp(step_count * (1000 / SEND_RATE_HZ));

    // 2. Simulate IMU (Sine waves to look like movement)
    auto* imu = packet.mutable_imu_data();
    auto* accel = imu->mutable_acceleration();
    auto* gyro = imu->mutable_angular_velocity();

    float time_sec = step_count * 0.01f;

    // Accel: Gravity + some wobble
    accel->set_x(0.5f * std::sin(time_sec));
    accel->set_y(0.5f * std::cos(time_sec));
    accel->set_z(-9.81f + (0.1f * std::sin(time_sec * 5.0f)));

    // Gyro: Spinning slowly
    gyro->set_x(0.1f);
    gyro->set_y(0.0f);
    gyro->set_z(0.05f);

    // 3. Simulate Altimeter
    auto* alt = packet.mutable_altimeter_data();
    alt->set_pressure(101325.0f - (step_count * 0.1f)); // Rising slowly
    alt->set_altitude(10.0f + (step_count * 0.05f));
    alt->set_temperature(25.5f);
}

int main() {
    std::cout << "--- Starting SPI Flight Sim Test ---\n";

    // 1. Initialize SPI Driver
    Slayter::HAL::SpiConfig config;
    config.devicePath = SPI_DEVICE;
    config.speedHz = SPI_SPEED;
    
    Slayter::HAL::SpiDriver spi(config);

    if (!spi.open()) {
        std::cerr << "CRITICAL: Failed to open SPI device: " << SPI_DEVICE << "\n";
        return -1;
    }
    std::cout << "SPI Driver Open. Speed: " << SPI_SPEED << " Hz\n";

    // 2. Loop
    SensorPacket packet;
    std::vector<uint8_t> rx_buffer;
    int step = 0;

    while (TOTAL_PACKETS == 0 || step < TOTAL_PACKETS) {
        auto start_time = std::chrono::steady_clock::now();

        // --- A. Generate Physics ---
        update_physics_model(packet, step);

        // --- B. Serialize ---
        // Protobuf usually serializes to std::string, we need std::vector<uint8_t>
        std::string serialized_string;
        if (!packet.SerializeToString(&serialized_string)) {
            std::cerr << "Failed to serialize packet!\n";
            break;
        }

        // Convert string to vector (copy)
        // Optimization: In production, we can serialize directly to array, 
        // but this is safer for now.
        std::vector<uint8_t> tx_buffer(serialized_string.begin(), serialized_string.end());

        // --- C. Transmit ---
        if (!spi.transfer(tx_buffer, rx_buffer)) {
            std::cerr << "SPI Transfer Failed at step " << step << "\n";
        } else {
            // Optional: Print status every 50 steps
            if (step % 50 == 0) {
                std::cout << "[Step " << step << "] Sent " << tx_buffer.size() 
                          << " bytes. Payload: Z-Accel=" 
                          << packet.imu_data().acceleration().z() << "\n";
            }
        }

        // --- D. Maintain Rate (Sleep) ---
        step++;
        auto end_time = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        int sleep_ms = (1000 / SEND_RATE_HZ) - elapsed.count();

        if (sleep_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_ms));
        }
    }

    std::cout << "--- Test Complete ---\n";
    return 0;
}
