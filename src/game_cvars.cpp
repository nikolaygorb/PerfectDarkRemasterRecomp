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

REXCVAR_DEFINE_BOOL(pdr_log_glare_queries, false, "Debug",
                    "Debug: log the light glares' occlusion query results and waits")
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

REXCVAR_DEFINE_BOOL(pdr_vulkan_float24_depth, true, "GPU",
                    "On Vulkan, convert float24 depth in the pixel shader (depth_float24_convert_in_"
                    "pixel_shader, depth_float24_round, depth_bias_shader_offset), which light glares "
                    "need. Not applied on D3D12, where it breaks the depth of the gun and hands")
    .lifecycle(rex::cvar::Lifecycle::kRequiresRestart);

REXCVAR_DEFINE_INT32(pdr_glare_query_wait_ms, 30, "GPU",
                     "Light glares read their occlusion queries three frames later. With "
                     "occlusion_query = async or strict, wait up to this long for a result "
                     "that is not ready yet instead of hiding the glare, 0 = don't wait")
    .range(0, 100)
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

REXCVAR_DEFINE_BOOL(pdr_mouse_look, true, "Mouse",
                    "Native mouse look: on foot the mouse turns the camera directly (1:1, no "
                    "dead zone or response curve) instead of emulating the right stick. "
                    "Elsewhere (menus, hoverbike, CamSpy) the stick emulation stays in charge. "
                    "Needs mnk_mode and mnk_mouse")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_DOUBLE(pdr_mouse_sensitivity, 2.0, "Mouse",
                      "Mouse look sensitivity: camera turn per mouse count = 0.022 deg x this "
                      "(as in Source games)")
    .range(0.01, 100.0)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(pdr_mouse_invert_x, false, "Mouse", "Invert the horizontal mouse look axis")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(pdr_mouse_invert_y, false, "Mouse", "Invert the vertical mouse look axis")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(pdr_mouse_zoom_scaling, true, "Mouse",
                    "Scale mouse look with the camera's field of view, so zoomed aiming turns "
                    "the picture by the same amount per mouse count")
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_DOUBLE(pdr_mouse_zoom_sensitivity, 1.0, "Mouse",
                      "Extra mouse look factor while zoomed in; the turn is already scaled to "
                      "the FOV")
    .range(0.01, 10.0)
    .lifecycle(rex::cvar::Lifecycle::kHotReload);

REXCVAR_DEFINE_BOOL(pdr_skip_live_prompt, false, "Gameplay",
                    "Answer the startup \"Gamer Profile not online\" prompt with "
                    "\"Continue playing offline\" (headless would pick \"Connect\" and hang)");

// Enable runtime debug tools (stub sweep, missing function scan)
REXCVAR_DEFINE_BOOL(dev_debug_runtime, false, "Debug",
                    "Enable runtime debug tools (stub sweep, missing function scan)");
