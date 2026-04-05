#include <flight_sim.hpp>

uint8_t* Serialize::serialize(const imu_data_t& data, uint8_t *buffer) {
    
    // 144 bytes
    // Needs to be in NanoPB decodable format
    // Thus I am applying keys ( key = (field_num << 3 | wire_type = 0) for wire_type = varint)
    // This is just key = counting by 0x08 for each field

    // Serialize Euler Angles Data
    buffer[0] = 0x08;
    buffer[1] = data.euler_angles.x.lsb & 0xFF;
    buffer[2] = 0x10;
    buffer[3] = data.euler_angles.x.msb & 0xFF;
    buffer[4] = 0x18;
    buffer[5] = data.euler_angles.y.lsb & 0xFF;
    buffer[6] = 0x20;
    buffer[7] = data.euler_angles.y.msb & 0xFF;
    buffer[8] = 0x28;
    buffer[9] = data.euler_angles.z.lsb & 0xFF;
    buffer[10] = 0x30;
    buffer[11] = data.euler_angles.z.msb & 0xFF;

    // Serialize Linear Acceleration Data
    buffer[12] = 0x38;
    buffer[13] = data.linear_acceleration.x.lsb & 0xFF;
    buffer[14] = 0x40;
    buffer[15] = data.linear_acceleration.x.msb & 0xFF;
    buffer[16] = 0x48;
    buffer[17] = data.linear_acceleration.y.lsb & 0xFF;
    buffer[18] = 0x50;
    buffer[19] = data.linear_acceleration.y.msb & 0xFF;
    buffer[20] = 0x58;
    buffer[21] = data.linear_acceleration.z.lsb & 0xFF;
    buffer[22] = 0x60;
    buffer[23] = data.linear_acceleration.z.msb & 0xFF;

    // Serialize Gyro Data
    buffer[24] = 0x68;
    buffer[25] = data.gyro.x.lsb & 0xFF;
    buffer[26] = 0x70;
    buffer[27] = data.gyro.x.msb & 0xFF;
    buffer[28] = 0x78;    
    buffer[29] = data.gyro.y.lsb & 0xFF;
    buffer[30] = 0x80;
    buffer[31] = data.gyro.y.msb & 0xFF;
    buffer[32] = 0x88;
    buffer[33] = data.gyro.z.lsb & 0xFF;
    buffer[34] = 0x90;
    buffer[35] = data.gyro.z.msb & 0xFF;

    return buffer;
}