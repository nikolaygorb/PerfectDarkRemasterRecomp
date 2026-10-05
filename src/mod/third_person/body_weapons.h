// The gun in the third-person body's hands follows the first-person gun.
#pragma once

#include <cstdint>

#include <rex/hook.h>

namespace third_person
{
  // Call once per tick while the body exists. Multiplayer swaps the held gun from
  // the gun switch code, but only while mplayerisrunning; in solo we do it here.
  void SyncBodyWeapons(PPCContext &ctx, uint8_t *base, uint32_t player);
} // namespace third_person
