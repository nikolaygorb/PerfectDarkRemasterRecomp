// With pdr_skip_live_prompt, the startup "Gamer Profile not online" box ("Connect
// to Xbox LIVE" / "Continue playing offline") answers itself with "Continue
// playing offline" instead of showing up.
//
// The profile is always signed in locally only, so the box comes up every start.
// headless mode picks the focused button, "Connect to Xbox LIVE", and the sign-in
// state machine (pd_platform_tick_signin) then waits for a LIVE connection that
// never comes. Other message boxes are left alone.
#include <rex/logging.h>

#include "../game/functions.h"
#include "../game/platform.h"
#include "../game_cvars.h"
#include "../guest/memory.h"

namespace platform = pd::platform;

// show_message_box(box, title, text, button1, button2, user) starts the box only
// while box->answer is nonzero (no box up) and clears it; the caller then waits
// for the answer.
REX_HOOK_RAW(pd_platform_show_message_box)
{
  const uint32_t box = ctx.r3.u32;
  const uint32_t answer = box + platform::kMessageBoxAnswer;
  if (!REXCVAR_GET(pdr_skip_live_prompt) || ctx.r4.u32 != platform::kStringProfileNotOnlineTitle ||
      ctx.r5.u32 != platform::kStringProfileNotOnlineText || guest::Load32(base, answer) == 0)
  {
    __imp__pd_platform_show_message_box(ctx, base);
    return;
  }

  REXLOG_INFO("Skipped the Xbox LIVE prompt: continuing offline");
  guest::Store32(base, answer, platform::kAnswerSecondButton);
}
