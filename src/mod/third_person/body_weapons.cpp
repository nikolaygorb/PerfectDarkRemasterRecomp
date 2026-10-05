#include "body_weapons.h"

#include "../../game/functions.h"
#include "../../game/player.h"
#include "../../game/prop.h"
#include "../../guest/memory.h"

namespace third_person
{
  namespace
  {
    using namespace pd::player;

    // Weapon each hand of the body holds, as last synced, and the body it was
    // synced for. A hand stays pending from deleting the old gun until it is empty.
    uint32_t g_body_model = 0;
    int32_t g_weapon[2] = {};
    bool g_pending[2] = {};

    int32_t WeaponNum(PPCContext &ctx, uint8_t *base, uint32_t hand)
    {
      ctx.r3.u64 = hand;
      pd_gun_get_weapon_num(ctx, base);
      return ctx.r3.s32;
    }
  } // namespace

  // Deleting only flags the held gun; the body's next chr tick frees it and empties
  // the hand, and creating does nothing while the hand is full. Multiplayer gets the
  // gap for free (delete on unequip, create on equip), so we wait for it.
  void SyncBodyWeapons(PPCContext &ctx, uint8_t *base, uint32_t player)
  {
    const uint32_t model = guest::Load32(base, player + kModel);
    if (!model)
    {
      g_body_model = 0;
      return;
    }
    if (model != g_body_model)
    {
      // A new body is created holding the right-hand gun only.
      g_body_model = model;
      g_weapon[kHandRight] = WeaponNum(ctx, base, kHandRight);
      g_weapon[kHandLeft] = 0;
      g_pending[kHandRight] = g_pending[kHandLeft] = false;
    }

    const uint32_t chr = guest::Load32(base, guest::Load32(base, player + kProp) + pd::prop::kChr);
    for (uint32_t hand : {kHandRight, kHandLeft})
    {
      const int32_t weapon = WeaponNum(ctx, base, hand);
      if (weapon != g_weapon[hand])
      {
        g_weapon[hand] = weapon;
        g_pending[hand] = true;
        ctx.r3.u64 = hand;
        pd_playermgr_delete_weapon(ctx, base);
      }
      if (g_pending[hand] && chr && guest::Load32(base, chr + pd::chr::kWeaponsHeld + hand * 4) == 0)
      {
        g_pending[hand] = false;
        ctx.r3.u64 = hand;
        pd_playermgr_create_weapon(ctx, base);
      }
    }
  }
} // namespace third_person
