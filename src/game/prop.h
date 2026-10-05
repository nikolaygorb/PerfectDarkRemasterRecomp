// prop.h - struct prop and struct chrdata
#pragma once

#include <cstdint>

namespace pd::prop
{
  constexpr uint32_t kChr = 4;
} // namespace pd::prop

namespace pd::chr
{
  constexpr uint32_t kModel = 32;
  constexpr uint32_t kWeaponsHeld = 368; // struct prop *weapons_held[hand]
} // namespace pd::chr
