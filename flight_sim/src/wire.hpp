#pragma pack(push,1)
struct SpiFrame {
  uint16_t preamble;     // 0xA55A
  uint8_t  version;      // 1
  uint8_t  type;         // 1 = IMU
  uint32_t seq;
  uint32_t ts_us;        // or ms
  float    ax, ay, az;
  float    gx, gy, gz;
  float    pressure, altitude, temperature;
  uint32_t crc32;        // over bytes [preamble..last float]
};
#pragma pack(pop)
static_assert(sizeof(SpiFrame)==52, "spiframe size");
uint32_t crc32_le(const void *data, size_t n);

