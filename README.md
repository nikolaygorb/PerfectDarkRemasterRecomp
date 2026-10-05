# Perfect Dark Remaster Recompilation

<div align="center">
  <img src="assets/icon.png" alt="Perfect Dark Remaster Recomp" width="480">
</div>

A static recompilation of **Perfect Dark Remaster** (2010, Xbox 360, Title ID `584109C2`) to native Windows x86-64, built on the [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk).

Static recompilation translates the Xbox 360 PowerPC code inside the game's `default.xex` into native C++ that compiles and runs on a PC. There is no emulator and no interpreter in the loop; file I/O, GPU commands, audio and threading go through the ReXGlue runtime.

> This project is not affiliated with Microsoft/Rare/4J. It contains no game
> files: you need your own legally owned copy of the game.

## Status

- **Working build.** The project compiles cleanly and the executable boots to real Direct3D 12 rendering with audio, controller support, and the texture-noise fix applied.
- **Codegen runs clean.** Analysis of `assets/default.xex` produces 0 errors; the code range `0x820C0000-0x825BB934` (≈ 4.8 MB) comes out as 15 032 recompiled functions.
- **Both graphics backends compiled in.** The SDK is built from source with `REXGLUE_USE_D3D12` and `REXGLUE_USE_VULKAN` both enabled, so `--graphics_backend=d3d12|vulkan` can be switched at runtime.
- **SDK built from source.** Unlike projects that vendor a prebuilt SDK archive, this repo pulls the ReXGlue SDK as a git submodule at [`thirdparty/rexglue-sdk`](thirdparty/rexglue-sdk) and builds it together with the game, so Tracy profiling and the SDK's perf counters (on in every non-Release build) come with it.
- **Debug instrumentation wired up.** `win-amd64-relwithdebinfo` builds enable `REXGLUE_ENABLE_PERF_COUNTERS` and link `TracyClientrd.dll`, so per-frame CSV metrics and Tracy profiling both work out of the box. See [`tools/benchmark.ps1`](tools/benchmark.ps1) and [`tools/analyze_benchmark.py`](tools/analyze_benchmark.py).

## Features

- **Native Windows build** — no emulator, no interpreter; the game's own PowerPC code is compiled to x86-64.
- **Third-person camera mod** — camera behind Joanna, her animated body with the gun she holds, reload animation, wall collision; toggle with `V`. Solo play. How it was made: [`src/mod/third_person/README.md`](src/mod/third_person/README.md).
- **No hang on mission restart** — the N64 audio library's event queue can run empty, and its voice handlers then spin forever holding the sound lock (black screen); an empty queue now yields a short delay instead ([`src/audio/event_queue.cpp`](src/audio/event_queue.cpp)).
- **Optional Xbox LIVE prompt skip** — `pdr_skip_live_prompt = true` answers the startup "Gamer Profile not online" box with "Continue playing offline" instead of showing it (`headless` would pick "Connect to Xbox LIVE" and hang).
- **Dual graphics backend** — pick Direct3D 12 or Vulkan at runtime via `--graphics_backend` or `settings/hardware.toml`.
- **Build optimizations** — Release builds use `-O3` with ThinLTO (and `-march=x86-64-v2` on x86-64), RelWithDebInfo `-O2 -g`; see [`cmake/build_optimizations.cmake`](cmake/build_optimizations.cmake).
- **Texture noise fix** — ported from the community xenia-canary `game-patches` for Perfect Dark (`584109C2`), applied as a direct guest-memory write in [`src/game_patches.h`](src/game_patches.h). Enabled by default in [`settings/hardware.toml`](settings/hardware.toml).
- **Runtime debug tools** — stub sweep and missing-function scan gated behind the `dev_debug_runtime` cvar; produces `logs/stub_sweep.txt` and `logs/missed_functions.txt` for ongoing work on unregistered addresses. A thread sampler (`pdr_sample_threads_after_s` / `pdr_sample_threads_duration_s`) logs the busiest threads' hottest functions once, `pdr_log_gpu_wait` logs render-thread GPU waits, and [`tools/thread_cpu.ps1`](tools/thread_cpu.ps1) prints per-thread CPU use of a running game.
- **Performance benchmarking** — `tools/benchmark.ps1` runs the game on both backends for a fixed duration and prints a side-by-side comparison; `tools/analyze_benchmark.py` computes FPS / frame-time percentiles from the CSV.
- **SDL gamepad support** — `settings/gamecontrollerdb.txt` is staged next to the exe so controllers SDL doesn't already recognize still work.

## Requirements

- CMake 3.25+
- Ninja
- Clang / LLVM (clang-cl works too) — MSVC alone will not build this
- Windows SDK (for D3D12) and Vulkan SDK (for Vulkan)
- Git with submodule support (the SDK is a submodule)
- Your own legally-owned copy of **Perfect Dark Remaster**, extracted from the Xbox One disc/ISO

## Getting the SDK

The SDK is a git submodule at [`thirdparty/rexglue-sdk`](thirdparty/rexglue-sdk). After cloning:

```powershell
git submodule update --init --recursive
```

That's it — `CMakeLists.txt` already points `REXSDK_DIR` at the submodule, so the build picks it up automatically.

## Getting the game data

1. Extract the Xbox One ISO with [XBLA-Extract](https://github.com/ryzendew/XBLA-Extract) (or a similar tool such as [xplorer360](https://github.com/XboxDev/xplorer360/releases) / [extract-xiso](https://github.com/XboxDev/extract-xiso/releases)).
2. Copy the extracted contents into `assets/`, so it looks like:
   ```
   assets/
     default.xex
     DataFiles/
     ...
   ```
3. `assets/` is gitignored (`assets/*` in `.gitignore`) — nothing from the disc is, or should be, committed to this repo.

## Build

```powershell
cmake --preset win-amd64-relwithdebinfo
cmake --build out/build/win-amd64-relwithdebinfo
```

Other available presets: `win-amd64-debug`, `win-amd64-release`, `win-arm64-*`, `linux-amd64-*`, `mac-amd64-*`, `mac-arm64-*`.

Codegen (translating `assets/default.xex` into `generated/default/*.cpp`) runs automatically as a build step (`perfectdarkremasterrecomp_codegen` CMake target) whenever `perfectdarkremasterrecomp_manifest.toml`, `default_functions.toml`, `function_names.toml`, or the `.xex` itself changes.

`function_names.toml` is generated, don't edit it by hand.

## Run

```powershell
cd out\build\win-amd64-relwithdebinfo
.\perfectdarkremasterrecomp.exe
```

Both `--game_data_root` and `--gpu_plugin` are optional: `OnConfigurePaths()` in [`src/perfectdarkremasterrecomp_app.h`](src/perfectdarkremasterrecomp_app.h) defaults `game_data_root` to `<repo_root>/assets` (found by walking up from the exe folder) when it isn't set via flag/env var, and `gpu_plugin = "xenos"` already lives in [`settings/hardware.toml`](settings/hardware.toml). Pass either flag to override.

Useful flags/env vars while developing:

| Flag / env var | Effect |
|---|---|
| `--game_data_root <path>` | Overrides the default `<repo_root>/assets` game-files location. |
| `--graphics_backend d3d12\|vulkan\|any` | Forces the graphics API `rexgpu-xenos` uses. Default `"any"` picks D3D12 first. |
| `--gpu_plugin xenos` | Overrides `settings/hardware.toml`'s `gpu_plugin`. |
| `--pdr_gpu_wait_mode 0\|1\|2` | Render thread wait for the GPU: busy spin, yield (default), sleep. |
| `--pdr_tp_enable true` | Third-person camera on at start (or press `V` in game); offsets: `pdr_tp_offset_x/y/z`. |
| `--perf_log_csv=<path>` | Per-frame metrics CSV (only when built with `REXGLUE_ENABLE_PERF_COUNTERS`). |

Logs are written to `out\build\<preset>\logs\*.log` (the exe is built `WIN32`, so nothing prints to the console).

## Configuration

Rendering/window/vsync and input-backend defaults are checked in under [`settings/`](settings/README.md) (`hardware.toml` / `mapping.toml`), loaded automatically at startup. CLI flags and `REX_*` environment variables always override them — see [`settings/README.md`](settings/README.md) for the full reference and precedence rules.

## Project structure

```
.
├── assets/                     # Game data (default.xex + files) — gitignored
├── cmake/
│   ├── build_optimizations.cmake # Compiler flags per build type (O3 + ThinLTO for Release)
│   └── fix_symlinks.cmake      # Fixes git symlinks on Windows (libmspack in SDK)
├── generated/                  # Codegen output (gitignored) + rexglue.cmake
│   └── default/                #   *.cpp / *.h per partition
├── out/                        # Build output (gitignored)
├── settings/                   # Checked-in hardware.toml / mapping.toml / gamecontrollerdb.txt
├── src/
│   ├── main.cpp                # REX_DEFINE_APP entry point
│   ├── perfectdarkremasterrecomp_app.h   # App hooks (OnPreSetup, OnPostSetup, ...)
│   ├── game_constants.h        # Code-range constants (kCodeBase / kCodeEnd)
│   ├── game_cvars.h/.cpp       # Project-defined cvars (graphics_backend, pdr_tp_*, ...)
│   ├── game/                   # What the host code knows about the game: struct offsets
│   │                           #   (g_vars, player, prop, ...) and the guest functions it uses
│   ├── guest/memory.h          # Big-endian guest memory access
│   ├── math/vec3.h             # Vector math
│   ├── audio/event_queue.cpp   # Fix: empty audio event queue no longer locks up the game
│   ├── mod/third_person/       # Third-person camera mod, one file per job
│   ├── platform/live_prompt.cpp # Optional Xbox LIVE prompt skip (continue offline)
│   ├── render/gpu_wait_hook.cpp # GPU ring-space wait: yield/sleep instead of busy spin
│   ├── game_patches.h          # Texture-noise fix (guest-memory write)
│   ├── debug_tools.h           # Stub sweep + missing-function scan
│   ├── thread_sampler.h/.cpp   # One-shot sampler of the busiest threads (debug)
│   └── utils.h                 # Repo-root discovery + settings loading
├── thirdparty/rexglue-sdk/     # ReXGlue SDK (git submodule)
├── tools/
│   ├── benchmark.ps1           # D3D12 vs Vulkan benchmark runner
│   ├── benchmark.sh            # Same, for Linux/macOS
│   ├── analyze_benchmark.py    # FPS / frame-time statistics from CSV
│   └── thread_cpu.ps1          # Per-thread CPU use of a running game
├── CMakeLists.txt
├── CMakePresets.json           # Platform presets (win/linux/mac × amd64/arm64)
├── CMakeUserPresets.json       # Local presets (inherits platform presets)
├── default_functions.toml      # Manual function boundaries for codegen
├── function_names.toml         # Function names for codegen (generated)
└── perfectdarkremasterrecomp_manifest.toml
```

## How this project was set up

1. `rexglue init --project-name perfectdarkremasterrecomp --xex-path assets/default.xex` generated `CMakeLists.txt`, `CMakePresets.json`, `perfectdarkremasterrecomp_manifest.toml`, `generated/rexglue.cmake`, `src/main.cpp`, `src/perfectdarkremasterrecomp_app.h`.
2. Added the ReXGlue SDK as a git submodule at `thirdparty/rexglue-sdk` and pointed `REXSDK_DIR` at it in `CMakeLists.txt` — the SDK is built from source rather than a prebuilt archive.
3. Enabled both graphics backends (`REXGLUE_USE_D3D12` on Windows, `REXGLUE_USE_VULKAN` everywhere) and set per-build-type compiler flags in [`cmake/build_optimizations.cmake`](cmake/build_optimizations.cmake).
4. Added `GPU_PLUGINS xenos` to the `rexglue_setup_target()` call in `CMakeLists.txt` so `rexgpu-xenos*.dll` gets staged next to the exe.
5. Added `src/game_patches.h` with the texture-noise fix (ported from xenia-canary `game-patches` for `584109C2`), applied in `OnPostLoadXexImage()`.
6. Added `src/debug_tools.h` with the stub sweep and missing-function scan, gated behind the `dev_debug_runtime` cvar so they don't run by default.
7. Added benchmarking tools (`tools/benchmark.ps1`, `tools/analyze_benchmark.py`).
8. Named the recompiled functions from the Perfect Dark decompilation and built the third-person camera mod on top ([`src/mod/third_person/README.md`](src/mod/third_person/README.md)).

## Known issues and difficulties encountered

- **Multiplayer / Co-op** is not implemented.
- **The `rexglue` CLI must be invoked through the SDK source tree.** Because the SDK is built from source, the `rexglue` binary is produced by the same build as the game; running codegen manually requires the SDK to have been built first.

## Credits

- [ReXGlue SDK](https://github.com/rexglue/rexglue-sdk) ([releases](https://github.com/rexglue/rexglue-sdk/releases))
- [xenia](https://github.com/xenia-project/xenia) / [xenia-canary](https://github.com/xenia-canary/xenia-canary) — ReXGlue's runtime is derived from Xenia's, and the [compatibility list](https://github.com/xenia-canary/game-compatibility/issues) was used to help pick this game as a recompilation target.
- [mdqinc/SDL_GameControllerDB](https://github.com/mdqinc/SDL_GameControllerDB) — controller mappings staged next to the exe.
- [ryzendew/XBLA-Extract](https://github.com/ryzendew/XBLA-Extract) — used to unpack the Xbox One ISO.
- [zerokilo/xexloaderwv](https://github.com/zerokilo/xexloaderwv) — Ghidra plugin used to load and analyze `default.xex` early on.
- [n64decomp/perfect_dark](https://github.com/n64decomp/perfect_dark) (MIT, Ryan Dwyer) — the N64 decompilation; 4J's port shares its code, which gave the function names and made the third-person mod possible. Not included in this repo.
