// vec3.h - 3D vector for host-side math on game coordinates
#pragma once

#include <cmath>

namespace math
{
  struct Vec3
  {
    float x, y, z;

    Vec3 operator+(const Vec3 &o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3 &o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    Vec3 Normalized() const
    {
      const float len = Length();
      return len > 1e-6f ? *this * (1.0f / len) : *this;
    }
  };

  inline Vec3 Cross(const Vec3 &a, const Vec3 &b)
  {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
  }
} // namespace math
