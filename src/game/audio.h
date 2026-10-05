// audio.h - The N64 audio library (naudio) the game's music and sounds run on
#pragma once

#include <cstdint>

namespace pd::audio
{
  // ALEventQueue
  constexpr uint32_t kEvtqAllocListNext = 8; // first queued event, 0 when empty

  // N_ALEvent
  constexpr uint32_t kEventType = 0; // s16
  constexpr uint16_t kEventTypeNone = 0xFFFF; // -1: what an empty queue hands out
} // namespace pd::audio
