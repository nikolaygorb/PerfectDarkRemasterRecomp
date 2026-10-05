// The third-person camera: the eye moved by a camera-local offset, pulled in front
// of walls with the game's own line-of-sight test.
#include <cmath>

#include "../../game/collision.h"
#include "../../game/functions.h"
#include "../../game/g_vars.h"
#include "../../game_cvars.h"
#include "../../guest/memory.h"
#include "third_person.h"

namespace
{
  using math::Vec3;
  namespace collision = pd::collision;

  // player_tick (normal tick mode) -> player_move_camera_from_pos_rooms
  constexpr uint32_t kMoveCameraFromTick = 0x821005E4;

  constexpr uint32_t kCdTypes = collision::kTypeBg | collision::kTypeClosedDoors;
  constexpr uint32_t kGeoFlags = collision::kGeoFloor1 | collision::kGeoFloor2 |
                                 collision::kGeoWall | collision::kGeoBlockSight;
  constexpr float kWallMargin = 12.0f; // keep the near plane out of the wall
  constexpr float kEaseOut = 0.2f;     // per frame, when the camera moves back out

  // Smoothed camera distance (fraction of the full offset): snaps in when a wall
  // gets close, eases back out when it is gone.
  float g_fraction = 1.0f;
  bool g_camera_moved = false;

  // How far along eye -> target the camera can go before it hits a wall (0..1).
  float FreeFraction(PPCContext &ctx, uint8_t *base, const Vec3 &eye, const Vec3 &target,
                     uint32_t basepos, uint32_t baserooms)
  {
    const float length = (target - eye).Length();
    if (length < 1.0f || basepos == 0 || baserooms == 0)
    {
      return 1.0f;
    }

    // Scratch below the caller's frame; the callees build their frames under sp.
    rex::CallFrame frame(ctx);
    const uint32_t sp = (ctx.r1.u32 - 0x400) & ~0xFu;
    const uint32_t from = sp + 0x200;
    const uint32_t to = sp + 0x210;
    const uint32_t eyerooms = sp + 0x220; // RoomNum[16], -1 terminated
    frame.ctx.r1.u64 = sp;
    guest::StoreVec3(base, from, eye);
    guest::StoreVec3(base, to, target);
    for (uint32_t i = 0; i < 16; ++i)
    {
      guest::Store16(base, eyerooms + i * 2, 0xFFFF);
    }

    // The eye can be in a different room than the body (basepos).
    frame.ctx.r3.u64 = basepos;
    frame.ctx.r4.u64 = from;
    frame.ctx.r5.u64 = baserooms;
    frame.ctx.r6.u64 = eyerooms;
    frame.ctx.r7.u64 = 0;
    frame.ctx.r8.u64 = 0;
    pd_portal_find_rooms(frame.ctx, base);
    if (guest::Load16(base, eyerooms) == 0xFFFF)
    {
      return 1.0f;
    }

    frame.ctx.r3.u64 = from;
    frame.ctx.r4.u64 = eyerooms;
    frame.ctx.r5.u64 = to;
    frame.ctx.r6.u64 = kCdTypes;
    frame.ctx.r7.u64 = kGeoFlags;
    pd_collision_test_los_oobok_findclosest(frame.ctx, base);
    if (frame.ctx.r3.u32 != collision::kResultCollision)
    {
      return 1.0f;
    }

    const float hit = (guest::LoadVec3(base, collision::kObstaclePos) - eye).Length();
    return std::fmax(0.0f, std::fmin(1.0f, (hit - kWallMargin) / length));
  }
} // namespace

// player_move_camera_from_pos_rooms(pos, up, look, basepos, baserooms). pos is the
// caller's local copy of the eye; the original derives the camera rooms from
// basepos -> pos, so they follow the moved eye.
REX_HOOK_RAW(pd_player_move_camera_from_pos_rooms)
{
  const uint32_t player = ctx.lr == kMoveCameraFromTick ? third_person::ActivePlayer(base) : 0;
  if (!player || pd::g_vars::TickMode(base) != pd::g_vars::kTickModeNormal)
  {
    g_camera_moved = false;
    __imp__pd_player_move_camera_from_pos_rooms(ctx, base);
    return;
  }

  const uint32_t pos = ctx.r3.u32;
  const Vec3 eye = guest::LoadVec3(base, pos);
  const Vec3 up = guest::LoadVec3(base, ctx.r4.u32).Normalized();
  const Vec3 look = guest::LoadVec3(base, ctx.r5.u32).Normalized();
  const Vec3 right = math::Cross(look, up).Normalized();
  const Vec3 offset = right * static_cast<float>(REXCVAR_GET(pdr_tp_offset_x)) +
                      up * static_cast<float>(REXCVAR_GET(pdr_tp_offset_y)) +
                      look * static_cast<float>(REXCVAR_GET(pdr_tp_offset_z));

  const float free = FreeFraction(ctx, base, eye, eye + offset, ctx.r6.u32, ctx.r7.u32);
  if (!g_camera_moved || free < g_fraction)
  {
    g_fraction = free;
  }
  else
  {
    g_fraction += (free - g_fraction) * kEaseOut;
  }
  g_camera_moved = true;

  guest::StoreVec3(base, pos, eye + offset * g_fraction);
  __imp__pd_player_move_camera_from_pos_rooms(ctx, base);
}
