# Third-person camera

Camera behind Joanna, her animated body with the gun she holds, reload animation,
camera pulled in front of walls. Toggle with `V` (`bind_third_person`); settings
`pdr_tp_*` in `settings/hardware.toml`. Solo play only.

## Why it was possible

Perfect Dark on Xbox 360 is 4J's port of the N64 code, and the N64 game has been
[decompiled](https://github.com/n64decomp/perfect_dark). 
So, we could read how the game itself works, and it
already has every piece a third-person view needs:

- **A body for the player.** While the CamSpy is in use, the player gets a body that
  is animated like a multiplayer opponent: walking, running, aiming, holding a gun.
- **A camera setter.** All camera positions go through one function
  (`player_move_camera_from_pos_rooms`), which also works out the camera's room.
- **Aiming from the camera.** Shots are traced from the camera through the
  crosshair, and the player's own body is skipped by hit tests. Moving the camera
  behind the player keeps aiming right with no extra work.

The mod mostly tells the game it is in situations it already handles, for the
length of one call:

| File | What it does |
|---|---|
| `body.cpp` | Keeps the body the normal tick would remove, and animates it as if the CamSpy were on. |
| `body_weapons.cpp` | Swaps the gun in the body's hands when the first-person gun changes. |
| `body_reload.cpp` | Plays the guards' reload animation while reloading (standing still). |
| `camera.cpp` | Moves the eye by a camera-local offset; the game's line-of-sight test stops it at walls. |
| `hud.cpp` | Skips drawing the first-person hands and gun. |

## How the pieces were found

1. The game image was decrypted straight from the `.xex`, so strings and
   constants could be read without running the game.
2. 4J left their source file names in assert strings (`.\bondview\bondview.cpp`)
   and debug field tables (`{"haschrbody", offset, size}`) in the image: functions
   could be placed in files, and struct offsets read off instead of guessed.
3. Recognisable code (the camera matrix built from eye and look vectors) led to the
   player tick and the camera setter; the decompilation explained the rest.
4. Functions were then matched to the decompilation wholesale (strings, function
   tables, call graph, code order), which is where the `pd_*` names come from.

## Difficulties

- **The body shares memory with the first-person gun in solo.** On the N64 there was
  no room for both, so making the body the solo way unloads the gun: no firing, no
  weapon switching. Fix: build the body the multiplayer way (own memory), by raising
  `mplayerisrunning` for that one call, and swap out a body the game already made
  the solo way (intro cutscene, CamSpy). At a mission start the memory can even be
  claimed for a body that never got made; it is handed back to the gun then.
- **Changing `cameramode` for good breaks things.** Third-person mode turns off
  firing sounds, footsteps, the HUD. So modes are only changed around single calls
  and put back right after.
- **Held guns are removed lazily.** Deleting the body's gun only flags it; the hand
  is empty a tick later. A new gun is only given once the hand is empty.
- **No reload animation for player bodies.** Multiplayer bodies never had one; the
  guards' whole-body animation is used, and only when standing still so the body
  doesn't slide.
- **No live testing on our side.** Everything was worked out from the code and the
  decompilation, then checked in game by hand, one round per build.
