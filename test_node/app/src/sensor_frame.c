#include "sensor_frame.h"
#include <pb_decode.h>
#include <string.h>

static bool fail(const char **error, const char *message)
{
    if (error) *error = message;
    return false;
}

bool sensor_frame_decode(const uint8_t *frame, size_t size,
                         SensorPacket *packet, const char **error)
{
    if (error) *error = NULL;
    if (!frame || !packet || size != 256) return fail(error, "frame must be 256 bytes");
    const size_t length = frame[0] | ((size_t)frame[1] << 8);
    if (length == 0 || length > 254) return fail(error, "invalid payload length");
    for (size_t i = 2 + length; i < size; ++i) {
        if (frame[i] != 0) return fail(error, "nonzero frame padding");
    }
    memset(packet, 0, sizeof(*packet));
    pb_istream_t stream = pb_istream_from_buffer(frame + 2, length);
    if (!pb_decode(&stream, SensorPacket_fields, packet)) return fail(error, "protobuf decode failed");
    if (packet->injection_mode != InjectionMode_IMMEDIATE)
        return fail(error, "MVP requires IMMEDIATE injection mode");
    if (!packet->has_imu || !packet->has_rc) return fail(error, "missing sensor payload");
    const int32_t axes[] = {
        packet->imu.euler_x, packet->imu.euler_y, packet->imu.euler_z,
        packet->imu.gyro_x, packet->imu.gyro_y, packet->imu.gyro_z,
        packet->imu.lin_acc_x, packet->imu.lin_acc_y, packet->imu.lin_acc_z
    };
    for (size_t i = 0; i < sizeof(axes) / sizeof(axes[0]); ++i) {
        if (axes[i] < -32768 || axes[i] > 32767) return fail(error, "IMU value exceeds int16");
    }
    if (packet->lidar_mm > 65535 || packet->rc.vertical < -127 || packet->rc.vertical > 127 ||
        packet->rc.horizontal < -127 || packet->rc.horizontal > 127)
        return fail(error, "LiDAR/RC value out of range");
    return true;
}
