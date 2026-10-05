// g_vars.h - g_Vars, the game's global state (offsets from the game's debug field tables)
#pragma once

#include <cstdint>

#include "../guest/memory.h"

namespace pd::g_vars
{
  constexpr uint32_t kAddress = 0x82648BF8;

  constexpr uint32_t kPlayers = 100; // struct player *players[4]
  constexpr uint32_t kCurrentPlayer = 1188;
  constexpr uint32_t kTickMode = 1228;
  constexpr uint32_t kMpIsRunning = 1332;
  constexpr uint32_t kUseNumPlayers = 1744; // nonzero: the player count is kNumPlayers
  constexpr uint32_t kNumPlayers = 1818;    // u8

  enum TickMode : uint32_t
  {
    kTickModeNormal = 1,
  };

  inline uint32_t CurrentPlayer(const uint8_t *base)
  {
    return guest::Load32(base, kAddress + kCurrentPlayer);
  }

  inline uint32_t TickMode(const uint8_t *base)
  {
    return guest::Load32(base, kAddress + kTickMode);
  }

  inline bool MpIsRunning(const uint8_t *base)
  {
    return guest::Load32(base, kAddress + kMpIsRunning) != 0;
  }

  // PLAYERCOUNT() (pd_playermgr_get_player_count).
  inline uint32_t PlayerCount(const uint8_t *base)
  {
    if (guest::Load32(base, kAddress + kUseNumPlayers) != 0)
    {
      return base[kAddress + kNumPlayers];
    }
    uint32_t count = 0;
    for (uint32_t i = 0; i < 4; ++i)
    {
      count += guest::Load32(base, kAddress + kPlayers + i * 4) != 0;
    }
    return count;
  }
} // namespace pd::g_vars
