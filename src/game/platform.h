// platform.h - 4J's platform layer: profiles, sign-in and system message boxes
#pragma once

#include <cstdint>

namespace pd::platform
{
  // A message box request (pd_platform_show_message_box's first argument).
  // The answer is 0 while the box is up, then which button closed it.
  constexpr uint32_t kMessageBoxAnswer = 160;

  enum Answer : uint32_t
  {
    kAnswerSecondButton = 3,
  };

  // Localized string ids of the startup "Gamer Profile not online" box:
  // "Connect to Xbox LIVE" / "Continue playing offline".
  constexpr uint32_t kStringProfileNotOnlineTitle = 0x8C74;
  constexpr uint32_t kStringProfileNotOnlineText = 0x8C75;
} // namespace pd::platform
