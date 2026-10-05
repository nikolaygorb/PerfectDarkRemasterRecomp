// collision.h - collision test types, geometry flags and the last-hit state
#pragma once

#include <cstdint>

namespace pd::collision
{
  // g_CdPos: where the last closest-hit test stopped.
  constexpr uint32_t kObstaclePos = 0x82629E48;

  // Collision results (pd_collision_test_*).
  constexpr uint32_t kResultCollision = 0;

  // CDTYPE_*
  constexpr uint32_t kTypeBg = 0x0020;
  constexpr uint32_t kTypeClosedDoors = 0x1000;

  // GEOFLAG_*
  constexpr uint32_t kGeoFloor1 = 0x0001;
  constexpr uint32_t kGeoFloor2 = 0x0002;
  constexpr uint32_t kGeoWall = 0x0004;
  constexpr uint32_t kGeoBlockSight = 0x0008;
} // namespace pd::collision
