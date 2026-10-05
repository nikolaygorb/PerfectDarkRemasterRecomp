// The third-person body: kept during normal play and animated like in multiplayer.
#include "../../game/functions.h"
#include "../../game/g_vars.h"
#include "../../game/player.h"
#include "../../guest/memory.h"
#include "body_weapons.h"
#include "third_person.h"

namespace
{
  using namespace pd::player;

  // player_tick (normal tick mode) -> player_remove_chr_body
  constexpr uint32_t kRemoveBodyFromTick = 0x82100540;
} // namespace

// player_remove_chr_body(): the normal tick removes the body every frame; we
// keep (or create) it instead.
REX_HOOK_RAW(pd_player_remove_chr_body)
{
  const uint32_t player = ctx.lr == kRemoveBodyFromTick ? third_person::ActivePlayer(base) : 0;
  if (!player)
  {
    __imp__pd_player_remove_chr_body(ctx, base);
    return;
  }

  // A body made the solo way (intro cutscene, CamSpy) sits in the first-person
  // gun's memory, so the gun can't load, fire or switch. Remove it like the game
  // does, which hands the memory back, and make our own below.
  if (guest::Load32(base, player + kHasChrBody) != 0 && guest::Load32(base, player + kGunMem2) != 0)
  {
    __imp__pd_player_remove_chr_body(ctx, base);
  }

  // The memory can also be claimed for a solo body that was never made (the
  // mission started before it was). The gun only gets it back once there is no
  // body at all, and ours stays, so the gun would stay empty: free it ourselves.
  if (base[player + kGunMemOwner] == kGunMemOwnerChrBody && guest::Load32(base, player + kGunMem2) == 0)
  {
    pd_gun_free_gun_mem(ctx, base);
  }

  // With mplayerisrunning raised, player_tick_chr_body loads the body the
  // multiplayer way: into memory of its own, not the first-person gun's.
  const uint32_t mp_flag = pd::g_vars::kAddress + pd::g_vars::kMpIsRunning;
  const uint32_t mp = guest::Load32(base, mp_flag);
  guest::Store32(base, mp_flag, 1);
  pd_player_tick_chr_body(ctx, base);
  guest::Store32(base, mp_flag, mp);

  third_person::SyncBodyWeapons(ctx, base, player);
}

// player_tick_third_person(prop): with cameramode EYESPY for the call, the body is
// animated like a multiplayer opponent (the multiplayer chr action) and marked
// onscreen.
REX_HOOK_RAW(pd_player_tick_third_person)
{
  const uint32_t prop = ctx.r3.u32;
  const uint32_t player = third_person::ActivePlayer(base);
  if (!player || guest::Load32(base, player + kProp) != prop ||
      guest::Load32(base, player + kHasChrBody) == 0 ||
      guest::Load32(base, player + kCameraMode) != kCameraModeDefault)
  {
    __imp__pd_player_tick_third_person(ctx, base);
    return;
  }

  guest::Store32(base, player + kCameraMode, kCameraModeEyespy);
  __imp__pd_player_tick_third_person(ctx, base);
  guest::Store32(base, player + kCameraMode, kCameraModeDefault);
}
