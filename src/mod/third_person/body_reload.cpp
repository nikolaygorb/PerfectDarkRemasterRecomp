// The third-person body plays the guards' reload animation while the first-person
// gun reloads. Multiplayer bodies have no reload animation of their own.
//
// The animation is a whole-body one (no separate upper body), so it plays only
// while the player stands still; moving switches straight back to the normal
// multiplayer animation instead of sliding the body along.
#include <cmath>

#include "../../game/anim.h"
#include "../../game/functions.h"
#include "../../game/player.h"
#include "../../game/prop.h"
#include "../../guest/memory.h"
#include "third_person.h"

namespace
{
  using namespace pd::player;

  constexpr float kStillSpeed = 0.1f; // |speedforwards|, |speedsideways| below this

  bool Reloading(const uint8_t *base, uint32_t player)
  {
    for (uint32_t hand : {kHandRight, kHandLeft})
    {
      const uint32_t h = player + kHands + hand * pd::hand::kSize;
      if (base[h + pd::hand::kInUse] != 0 &&
          guest::Load32(base, h + pd::hand::kState) == pd::hand::kStateReload)
      {
        return true;
      }
    }
    return false;
  }

  bool StandingStill(const uint8_t *base, uint32_t player)
  {
    return std::fabs(guest::LoadFloat(base, player + kSpeedForwards)) < kStillSpeed &&
           std::fabs(guest::LoadFloat(base, player + kSpeedSideways)) < kStillSpeed;
  }
} // namespace

// player_choose_third_person_animation(chr, ...): picks the body's animation every
// tick. While reloading we start the reload animation once and keep the call from
// replacing it; afterwards the original blends back to the usual animation.
REX_HOOK_RAW(pd_player_choose_third_person_animation)
{
  const uint32_t chr = ctx.r3.u32;
  const uint32_t player = third_person::ActivePlayer(base);
  if (!player ||
      chr != guest::Load32(base, guest::Load32(base, player + kProp) + pd::prop::kChr) ||
      !Reloading(base, player) || !StandingStill(base, player))
  {
    __imp__pd_player_choose_third_person_animation(ctx, base);
    return;
  }

  const uint32_t model = guest::Load32(base, chr + pd::chr::kModel);
  if (!model)
  {
    return;
  }
  ctx.r3.u64 = model;
  pd_model_get_anim_num(ctx, base);
  if ((ctx.r3.u32 & 0xFFFF) == pd::anim::kReload)
  {
    return; // already playing (or held on its last frame)
  }

  ctx.r3.u64 = model;
  ctx.r4.u64 = pd::anim::kReload;
  ctx.r5.u64 = 0; // no flip
  ctx.f1.f64 = 0.0;
  ctx.f2.f64 = pd::anim::kAiSpeed;
  ctx.f3.f64 = pd::anim::kAiMerge;
  pd_model_set_animation(ctx, base);
}
