// No first-person hands and gun while the third-person body is shown.
#include "../../game/functions.h"
#include "../../game/player.h"
#include "../../guest/memory.h"
#include "third_person.h"

namespace
{
  using namespace pd::player;

  // player_render_hud -> gun_render
  constexpr uint32_t kGunRenderFromHud = 0x820FC7B0;
} // namespace

// gun_render(&gdl): its X-ray branch returns before drawing (after the same rocket
// bookkeeping), so the call sees visionmode XRAY.
REX_HOOK_RAW(pd_gun_render)
{
  const uint32_t player = ctx.lr == kGunRenderFromHud ? third_person::ActivePlayer(base) : 0;
  if (!player || guest::Load32(base, player + kHasChrBody) == 0)
  {
    __imp__pd_gun_render(ctx, base);
    return;
  }

  const uint16_t vision = guest::Load16(base, player + kVisionMode);
  guest::Store16(base, player + kVisionMode, kVisionModeXray);
  __imp__pd_gun_render(ctx, base);
  guest::Store16(base, player + kVisionMode, vision);
}
