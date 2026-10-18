#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sensor_data.pb.h>

// Shared by the SPI receiver and the host nanopb compatibility test.
// MVP accepts only IMMEDIATE frames; legacy/scheduled frames are rejected.
bool sensor_frame_decode(const uint8_t *frame, size_t size,
                         SensorPacket *packet, const char **error);
