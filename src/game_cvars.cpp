#include "game_cvars.h"

REXCVAR_DEFINE_STRING(graphics_backend, "any", "GPU",
                      "Graphics API backend: any, d3d12, vulkan")
    .allowed({"any", "d3d12", "vulkan"});

// Experimental port of the community xenia-canary "Fix texture noise on
// NVIDIA GPUs" patch for Perfect Dark (584109C2). Off by default to match
// upstream.
REXCVAR_DEFINE_BOOL(pdr_texture_noise_fix, false, "Gameplay",
                    "Experimental: fix texture noise on NVIDIA GPUs (ported "
                    "from xenia-canary game-patches)");

REXCVAR_DEFINE_BOOL(pdr_log_gpu_wait, false, "Debug",
                    "Debug: log how long the render thread waits for command ring space")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_INT32(pdr_sample_threads_after_s, 0, "Debug",
                     "Debug: after this many seconds, sample the busiest threads once and log "
                     "their hottest functions, 0 = off")
    .range(0, 3600);
REXCVAR_DEFINE_INT32(pdr_sample_threads_duration_s, 10, "Debug",
                     "Debug: how long the thread sampler samples for")
    .range(1, 120);

REXCVAR_DEFINE_INT32(pdr_gpu_wait_mode, 1, "Performance",
                     "Pause between polls while the game waits for free command-buffer space: "
                     "0 = busy spin (original), 1 = yield the core, 2 = sleep 200us")
    .range(0, 2)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(pdr_tp_enable, false, "Gameplay",
                    "Third-person camera (solo only): camera behind Joanna, body shown, "
                    "first-person gun hidden")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_DOUBLE(pdr_tp_offset_x, 35.0, "Gameplay",
                      "Third-person camera offset, camera-local X (right)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_DOUBLE(pdr_tp_offset_y, 15.0, "Gameplay",
                      "Third-person camera offset, camera-local Y (up)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_DOUBLE(pdr_tp_offset_z, -170.0, "Gameplay",
                      "Third-person camera offset, camera-local Z (forward, < 0 = behind)")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(pdr_skip_live_prompt, false, "Gameplay",
                    "Answer the startup \"Gamer Profile not online\" prompt with "
                    "\"Continue playing offline\" (headless would pick \"Connect\" and hang)");

// Enable runtime debug tools (stub sweep, missing function scan)
REXCVAR_DEFINE_BOOL(dev_debug_runtime, false, "Debug",
                    "Enable runtime debug tools (stub sweep, missing function scan)");
