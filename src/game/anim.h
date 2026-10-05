// anim.h - Animation numbers (ANIM_*) and how the game plays them
#pragma once

#include <cstdint>

namespace pd::anim
{
  // The reload guards play (AI command chr_do_animation(ANIM_RELOAD_0209, ...)).
  constexpr uint16_t kReload = 0x0209;

  // AI animations run at 1 / the script's speed and blend over this many frames.
  constexpr float kAiSpeed = 0.5f;
  constexpr float kAiMerge = 16.0f;
} // namespace pd::anim
