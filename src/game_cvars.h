// game_cvars.h - Game-specific CVAR definitions
#pragma once

#include <rex/cvar.h>

REXCVAR_DEFINE_STRING(graphics_backend, "any", "GPU",
                      "Graphics API backend: any, d3d12, vulkan")
    .allowed({"any", "d3d12", "vulkan"});

// Experimental port of the community xenia-canary "Fix texture noise on
// NVIDIA GPUs" patch for Perfect Dark (584109C2). Off by default to match
// upstream.
REXCVAR_DEFINE_BOOL(pdr_texture_noise_fix, false, "Gameplay",
                    "Experimental: fix texture noise on NVIDIA GPUs (ported "
                    "from xenia-canary game-patches)");

// Enable runtime debug tools (stub sweep, missing function scan)
REXCVAR_DEFINE_BOOL(dev_debug_runtime, false, "Debug",
                    "Enable runtime debug tools (stub sweep, missing function scan)");
