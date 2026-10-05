// memory.h - Big-endian access to guest memory (base = virtual membase)
#pragma once

#include <cstdint>
#include <cstring>

#include "../math/vec3.h"

namespace guest
{
  inline uint32_t Load32(const uint8_t *base, uint32_t address)
  {
    const uint8_t *p = base + address;
    return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | p[3];
  }

  inline void Store32(uint8_t *base, uint32_t address, uint32_t value)
  {
    uint8_t *p = base + address;
    p[0] = uint8_t(value >> 24);
    p[1] = uint8_t(value >> 16);
    p[2] = uint8_t(value >> 8);
    p[3] = uint8_t(value);
  }

  inline uint16_t Load16(const uint8_t *base, uint32_t address)
  {
    const uint8_t *p = base + address;
    return uint16_t((p[0] << 8) | p[1]);
  }

  inline void Store16(uint8_t *base, uint32_t address, uint16_t value)
  {
    uint8_t *p = base + address;
    p[0] = uint8_t(value >> 8);
    p[1] = uint8_t(value);
  }

  inline float LoadFloat(const uint8_t *base, uint32_t address)
  {
    const uint32_t bits = Load32(base, address);
    float value;
    std::memcpy(&value, &bits, sizeof(value));
    return value;
  }

  inline void StoreFloat(uint8_t *base, uint32_t address, float value)
  {
    uint32_t bits;
    std::memcpy(&bits, &value, sizeof(bits));
    Store32(base, address, bits);
  }

  // struct coord: three floats.
  inline math::Vec3 LoadVec3(const uint8_t *base, uint32_t address)
  {
    return {LoadFloat(base, address), LoadFloat(base, address + 4), LoadFloat(base, address + 8)};
  }

  inline void StoreVec3(uint8_t *base, uint32_t address, const math::Vec3 &v)
  {
    StoreFloat(base, address, v.x);
    StoreFloat(base, address + 4, v.y);
    StoreFloat(base, address + 8, v.z);
  }
} // namespace guest
