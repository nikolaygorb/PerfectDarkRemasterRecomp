// Mouse look straight into the player's view angles.
//
// pd_move_process_input(allowc1x, allowc1y, allowc1buttons, ignorec2) reads the pad and
// turns it into speedtheta (player+336) and speedverta (+356); the vertical angle vv_verta
// (+348, degrees, + = up as in the N64 decomp) is integrated in it, the horizontal vv_theta (+332, degrees,
// 0..360, + = right) right after it in the walk tick (pd_walk_tick -> 0x821042C0). Both
// end in pd_move_update_look, which wraps and clamps vv_verta and derives the sines,
// cosines and look vectors from the two angles.
//
// player_tick calls the move tick (sub_820F4D40 -> pd_move_process_input) with
// allowc1x = allowc1y = 1 only in normal play; menus, death and the like pass 0, 0. So the
// hook adds the mouse turn to the angles after the original, only on that call, only on
// foot (movemode WALK) and outside the CamSpy. Mouse counts are an angle, not a rate: the
// stick's dead zone, response curve and speed cap are skipped, the vertical limits in
// pd_move_update_look still apply.
//
// g_ViData->fovy is the current vertical FOV (60 unzoomed). The turn is scaled by
// tan(fov / 2) / tan(unzoomed / 2), so the picture moves under the mouse at the same rate
// at the screen center whatever the zoom.
#include <algorithm>
#include <cmath>

#include "../../game/functions.h"
#include "../../game/g_vars.h"
#include "../../game/player.h"
#include "../../game_cvars.h"
#include "../../guest/memory.h"
#include "mouse_look.h"

namespace
{
  using namespace pd::player;

  constexpr uint32_t kViData = 0x825CC15C; // struct vidata *g_ViData
  constexpr uint32_t kViFovY = 8;          // f32, degrees
  constexpr double kUnzoomedFovY = 60.0;
  constexpr double kRadiansPerDegree = 3.14159265358979323846 / 180.0;

  bool PlausibleFov(double fov)
  {
    return std::isfinite(fov) && fov > 1.0 && fov < 170.0;
  }

  double ZoomScale(const uint8_t *base)
  {
    const uint32_t vidata = guest::Load32(base, kViData);
    if (vidata == 0 || !REXCVAR_GET(pdr_mouse_zoom_scaling))
    {
      return 1.0;
    }
    const double fov = guest::LoadFloat(base, vidata + kViFovY);
    if (!PlausibleFov(fov))
    {
      return 1.0;
    }
    const double scale = std::tan(fov * 0.5 * kRadiansPerDegree) /
                         std::tan(kUnzoomedFovY * 0.5 * kRadiansPerDegree);
    if (scale >= 0.999)
    {
      return 1.0;
    }
    return std::clamp(scale * REXCVAR_GET(pdr_mouse_zoom_sensitivity), 0.02, 1.0);
  }

  // The player the keyboard and mouse drive: the first one.
  bool IsMousePlayer(const uint8_t *base, uint32_t player)
  {
    return player != 0 &&
           guest::Load32(base, pd::g_vars::kAddress + pd::g_vars::kPlayers) == player;
  }
} // namespace

REX_HOOK_RAW(pd_move_process_input)
{
  const bool allow_look = ctx.r3.u32 != 0 && ctx.r4.u32 != 0;
  __imp__pd_move_process_input(ctx, base);

  auto &look = mouse_look::MouseLook::Get();
  if (!allow_look || !look.Enabled())
  {
    return;
  }
  const uint32_t player = pd::g_vars::CurrentPlayer(base);
  if (!IsMousePlayer(base, player) ||
      guest::Load32(base, player + kMoveMode) != kMoveModeWalk ||
      guest::Load32(base, player + kCameraMode) != kCameraModeDefault)
  {
    return; // the watchdog hands the mouse back to the stick emulation
  }
  if (!(pd::g_vars::LvUpdate60fReal(base) > 0.0f))
  {
    return; // paused: the motion stays pending for the next update
  }

  const auto turn = look.Consume(ZoomScale(base));
  if (!turn)
  {
    return;
  }

  float theta = guest::LoadFloat(base, player + kTheta) + static_cast<float>(turn->yaw);
  theta = std::fmod(theta, 360.0f);
  if (theta < 0.0f)
  {
    theta += 360.0f;
  }
  // pd_move_update_look clamps it to the game's own limits.
  const float verta = std::clamp(
      guest::LoadFloat(base, player + kVerta) + static_cast<float>(turn->pitch), -90.0f, 90.0f);
  guest::StoreFloat(base, player + kTheta, theta);
  guest::StoreFloat(base, player + kVerta, verta);
}
