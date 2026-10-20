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

  const uint32_t gdlptr = ctx.r3.u32;
  const uint16_t vision = guest::Load16(base, player + kVisionMode);
  guest::Store16(base, player + kVisionMode, kVisionModeXray);
  __imp__pd_gun_render(ctx, base);
  guest::Store16(base, player + kVisionMode, vision);
  if (vision == kVisionModeXray)
  {
    return;
  }

  // The X-ray branch also skips artifacts_tick and the light glares' occlusion
  // queries, so no glare would ever be drawn. Do what player_render_hud does in
  // place of gun_render for the game's own third-person camera (cameramode 1).
  pd_artifact_tick(ctx, base);
  ctx.r3.u64 = guest::Load32(base, gdlptr);
  ctx.r4.u64 = 0;
  pd_zbuf_save_artifact_depths_late(ctx, base);
  guest::Store32(base, gdlptr, ctx.r3.u32);
}
