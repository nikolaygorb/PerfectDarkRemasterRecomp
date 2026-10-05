#include "third_person.h"

#include <chrono>

#include <rex/ui/keybinds.h>

#include "../../game/g_vars.h"
#include "../../game_cvars.h"

namespace third_person
{
  uint32_t ActivePlayer(const uint8_t *base)
  {
    if (!REXCVAR_GET(pdr_tp_enable) || pd::g_vars::MpIsRunning(base) ||
        pd::g_vars::PlayerCount(base) != 1)
    {
      return 0;
    }
    return pd::g_vars::CurrentPlayer(base);
  }

  void RegisterBinds()
  {
    rex::ui::RegisterBind("bind_third_person", "V", "Toggle third-person camera", []
                          {
      // Held keys auto-repeat; only a press after a quiet gap counts.
      static auto last_event = std::chrono::steady_clock::time_point{};
      const auto now = std::chrono::steady_clock::now();
      const bool fresh_press = now - last_event > std::chrono::milliseconds(300);
      last_event = now;
      if (fresh_press)
      {
        REXCVAR_SET(pdr_tp_enable, !REXCVAR_GET(pdr_tp_enable));
      } });
  }
} // namespace third_person
