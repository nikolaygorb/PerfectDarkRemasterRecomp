# Settings

Two hand-authored, checked-in config files that the `PerfectDarkRemasterRecomp` binary loads
automatically at startup, plus a downloaded controller mapping database and
this README.

| File | Covers |
|---|---|
| [`hardware.toml`](hardware.toml) | Renderer/backend, window, vsync, resolution, debug output, gameplay patches and the third-person mod |
| [`mapping.toml`](mapping.toml) | Input backend selection (gamepad/keyboard+mouse), key bindings, mod hotkeys |
| [`gamecontrollerdb.txt`](gamecontrollerdb.txt) | SDL gamepad mappings for controllers it doesn't already recognize - not hand-authored, see below |

Split in two on purpose: rendering/perf tweaks and input tweaks are separate
concerns you'll want to touch independently (SOLID/DRY - one file per
concern instead of one growing junk-drawer file), and both stay tiny because
they only list the cvars this project actually cares about, not the full
~80-flag surface the SDK exposes (YAGNI).

**Format: flat `key = value` only, no `[Table]` headers.** Confirmed via
`rex::cvar::SerializeToTOML()`, which itself dumps a flat list - the loader
reads root-level keys, it does not recurse into nested tables. A key put
under a `[Section]` header is silently ignored (found the hard way: values
under `[Display]`/`[GPU]` headers never took effect until the headers were
removed).

## `hardware.toml` reference

| Key | Type | Default | Effect |
|---|---|---|---|
| `gpu_plugin` | string | `"xenos"` | GPU emulation plugin to load. Set here rather than on the command line now - see root [README](../README.md#run). A `--gpu_plugin` flag would still override this file if you ever needed a different plugin. |
| `graphics_backend` | string | `"any"` | Graphics API backend: `"any"` (D3D12 first when both are compiled in), `"d3d12"`, or `"vulkan"`. Project-defined cvar (not from the SDK), wired up in `OnPreSetup()` in [`src/perfectdarkremasterrecomp_app.h`](../src/perfectdarkremasterrecomp_app.h) - it loads the plugin itself with the requested backend before the SDK's own auto-load (which only ever requests `"any"`) runs. Falls back to automatic selection with a warning if the requested backend isn't compiled into `rexgpu-xenos`. |
| `vsync` | bool | `true` | Vertical sync. |
| `resolution_scale` | int | `1` | Internal render-target supersampling, independent of window/guest resolution. |
| `async_shader_compilation` | bool | `true` | Compile shaders on a background thread instead of blocking the render thread - reduces hitches when new shaders are first seen. |
| `native_2x_msaa` | bool | `false` | Native 2x MSAA on the emulated render targets. |
| `anisotropic_override` | int | `0` | Forces anisotropic texture filtering to this level (e.g. `16`); `0` leaves the game's own setting alone. |
| `window_width` / `window_height` | int | `1280` / `720` | Host window size. |
| `fullscreen` | bool | `false` | Host window fullscreen. |
| `monitor` | int | `0` | Host monitor index for fullscreen. |
| `resolution` | string | `"1280x720"` | Guest ("TV") video mode reported to the game - affects the game's own UI scale/aspect logic, separate from the host window size above. |
| `present_letterbox` | bool | `true` | Letterbox instead of stretch when window and guest aspect ratios differ. |
| `d3d12_debug` | bool | `false` | D3D12 debug layer. Leave off - noticeably slower. |
| `license_mask` | int | `0` | SDK cvar, read by `XamContentGetLicenseMask_entry` in `rexglue-sdk/src/kernel/xam/xam_content.cpp`. Each bit represents a granted license; bit 0 is "purchased". Default `0` makes XBLA titles run as trial and show "UNLOCK FULL VERSION". Set to `4294967295` (`0xFFFFFFFF`) to claim every license bit. |
| `headless` | bool | `false` | SDK cvar, read by `XamShowMessageBoxUI_entry` in `rexglue-sdk/src/kernel/xam/xam_ui.cpp`. When `true`, message-box dialogs auto-pick the active button instead of showing an ImGui dialog. Not for the startup "Gamer Profile not online" box: its active button is "Connect to Xbox LIVE", and the game then hangs waiting for LIVE - use `pdr_skip_live_prompt` for that one. |
| `pdr_texture_noise_fix` | bool | `false` | Experimental. Ported from the community xenia-canary `game-patches` "Fix texture noise on NVIDIA GPUs" patch for Perfect Dark (title `584109C2`). The address (`0x82014B7B`) is a plain data constant with no corresponding C++ statement, so it's applied as a direct guest-memory write by `ApplyTextureNoiseFix()` in [`src/game_patches.h`](../src/game_patches.h), called from `OnPostLoadXexImage()`. Skipped on Linux. Off by default, matching upstream. |
| `pdr_gpu_wait_mode` | int | `1` | How the render thread waits for free GPU command-ring space: `0` busy spin (original), `1` yield the core, `2` sleep 200 µs. |
| `pdr_tp_enable` | bool | `false` | Third-person camera mod (solo only); also toggled in game with `bind_third_person`. See [`src/mod/third_person/README.md`](../src/mod/third_person/README.md). |
| `pdr_tp_offset_x` / `_y` / `_z` | double | `35` / `15` / `-170` | Third-person camera offset from the eye, camera-local, in game units (~cm): right, up, forward (negative = behind). |
| `pdr_skip_live_prompt` | bool | `false` | Answers the startup "Gamer Profile not online" box with "Continue playing offline" instead of showing it. |
| `dev_debug_runtime` | bool | `false` | Debug: stub sweep and missing-function scan, writing `logs/stub_sweep.txt` and `logs/missed_functions.txt` (large). |
| `pdr_log_gpu_wait` | bool | `false` | Debug: logs how long the render thread waits for command-ring space. |
| `pdr_sample_threads_after_s` / `_duration_s` | int | `0` / `10` | Debug: after this many seconds (`0` = off), sample the busiest threads for the given time once and log their hottest functions. |

## `mapping.toml` reference

| Key | Type | Default | Effect |
|---|---|---|---|
| `input_backend` | string | `"sdl"` | `""` is rejected by validation (logs a warning, falls back to default) - must be an explicit `"sdl"` or `"xinput"`. |
| `hid_mappings_file` | string | `"gamecontrollerdb.txt"` | Extra SDL gamepad mappings loaded for controllers SDL doesn't already recognize, or `""` to skip. `gamecontrollerdb.txt` next to the exe is staged by `CMakeLists.txt` from [`gamecontrollerdb.txt`](gamecontrollerdb.txt) in this folder, sourced from [mdqinc/SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) - re-copy that file from upstream any time to refresh it. |
| `guide_button` | bool | `true` | Whether the controller Guide/PS button is routed to the guest. |
| `mnk_mode` | bool | `true` | Enables keyboard-as-controller input, merged in alongside any physical gamepad (see below). |
| `mnk_mouse` | bool | `true` | Routes mouse movement to the right stick when `mnk_mode` is on. Off means the right stick only comes from the `keybind_rstick_*` keys. |
| `mnk_sensitivity` | double | `1.0` | Mouse sensitivity for the right stick, range `0.01`-`10.0`. |
| `keybind_*` | string | | Keyboard keys for each controller button / stick direction (comma-separated alternatives, e.g. `"Z,Tab"`). |
| `bind_third_person` | string | `"V"` | Toggles the third-person camera. Registered by the app in `OnCreateDialogs()`, so it can be rebound here. |

Gamepad button layout itself (DualShock 4, DualSense, Xbox, Switch Pro, ...)
isn't a setting here - SDL auto-detects the controller and maps it to the
Xbox 360 layout the game expects.

## Adding more cvars

Any cvar declared with `REXCVAR_DECLARE` under
`thirdparty/rexglue-sdk/include/rex/**/flags.h` can be added the same way: pick
the file matching its concern (hardware vs. input), add `key = value`. If it
needs to apply before the window/overlays exist (true for everything
currently listed), it belongs in one of these two files loaded from
`OnConfigurePaths`. Hotkeys the app registers itself (`rex::ui::RegisterBind`,
like `bind_third_person`) go in `mapping.toml`; they must be registered before
`OnPostSetup` reloads the files, which is why that happens in `OnCreateDialogs`.
