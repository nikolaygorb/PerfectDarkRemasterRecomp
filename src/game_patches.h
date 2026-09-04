// game_patches.h - Game-specific memory patches
#pragma once

#include <cstdint>

#include <rex/cvar.h>
#include <rex/memory/utils.h>
#include <rex/runtime.h>

#include "game_cvars.h"

namespace game_patches
{
  // Apply the "Fix texture noise on NVIDIA GPUs" patch
  // Ported from xenia-canary game-patches for Perfect Dark (584109C2)
  // https://github.com/xenia-canary/game-patches/blob/5c3b70e92c1c050dafd9e35a6c57e1edf4fb1a47/patches/584109C2%20-%20Perfect%20Dark.patch.toml
  static void ApplyTextureNoiseFix()
  {
    if (!REXCVAR_GET(pdr_texture_noise_fix))
    {
      return;
    }

    auto *rt = rex::Runtime::instance();
    uint8_t *base = rt ? rt->virtual_membase() : nullptr;
    if (!base)
    {
      return;
    }

    constexpr uint32_t kTextureNoiseAddr = 0x82014b7b;
    constexpr uint8_t kTextureNoiseValue = 0x30;
    uint8_t *p = base + kTextureNoiseAddr;

    // The byte lives in a read-only-mapped section (write access violation
    // without this) - unprotect just long enough to patch it, then restore
    // whatever access it had before.
    rex::memory::PageAccess old_access{};
    rex::memory::Protect(p, 1, rex::memory::PageAccess::kReadWrite, &old_access);
    p[0] = kTextureNoiseValue;
    rex::memory::Protect(p, 1, old_access, nullptr);
  }
} // namespace game_patches
