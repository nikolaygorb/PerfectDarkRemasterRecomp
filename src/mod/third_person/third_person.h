// Third-person camera mod.
//
// The game already has what a third-person view needs: while the CamSpy is in use
// the player gets a body animated like a multiplayer opponent. The mod keeps that
// body during normal play and moves the camera behind it:
//   body.cpp         - keep the body (loaded the multiplayer way) and animate it
//   body_weapons.cpp - the gun in the body's hands follows the first-person gun
//   body_reload.cpp  - the reload animation, standing still
//   camera.cpp       - camera behind the eye, pulled in front of walls
//   hud.cpp          - no first-person hands and gun
// Shots are traced from the camera through the crosshair, and the player's own
// body is skipped by hit tests, so aiming needs no changes. Solo play only.
#pragma once

#include <cstdint>

namespace third_person
{
  // Registers the toggle hotkey (bind_third_person, default V).
  void RegisterBinds();

  // The current player when the mod applies (enabled, one player, not
  // multiplayer), else 0.
  uint32_t ActivePlayer(const uint8_t *base);
} // namespace third_person
