// player.h - struct player (offsets from the game's debug field tables)
#pragma once

#include <cstdint>

namespace pd::player
{
  constexpr uint32_t kCameraMode = 0;
  constexpr uint32_t kVisionMode = 16; // u16
  constexpr uint32_t kProp = 188;
  constexpr uint32_t kModel = 220; // model00d4: the third-person body model
  constexpr uint32_t kSpeedSideways = 368; // f32, -1..1
  constexpr uint32_t kSpeedForwards = 376; // f32, -1..1
  constexpr uint32_t kHands = 1632;        // struct hand hands[2]
  constexpr uint32_t kGunMemOwner = 5650; // u8, gunctrl.gunmemowner (GunMemOwner)
  constexpr uint32_t kHasChrBody = 6640;
  constexpr uint32_t kGunMem2 = 7164; // set while a solo-made body holds the gun memory

  // Who the first-person gun's memory belongs to (GUNMEMOWNER_*).
  enum GunMemOwner : uint8_t
  {
    kGunMemOwnerChrBody = 2,
    kGunMemOwnerFree = 11,
  };

  enum CameraMode : uint32_t
  {
    kCameraModeDefault = 0,
    kCameraModeEyespy = 2,
  };

  enum VisionMode : uint16_t
  {
    kVisionModeXray = 1,
  };

  enum Hand : uint32_t
  {
    kHandRight = 0,
    kHandLeft = 1,
  };
} // namespace pd::player

// struct hand: the first-person gun in one hand, player + kHands + hand * kSize.
namespace pd::hand
{
  constexpr uint32_t kSize = 1956;
  constexpr uint32_t kInUse = 8;     // s8: holding a gun
  constexpr uint32_t kState = 1540;  // HANDSTATE_*

  enum State : uint32_t
  {
    kStateReload = 1,
  };
} // namespace pd::hand
