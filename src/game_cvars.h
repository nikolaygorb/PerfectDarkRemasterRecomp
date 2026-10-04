// game_cvars.h - Game-specific CVAR declarations
#pragma once

#include <cstdint>
#include <string>

#include <rex/cvar.h>

REXCVAR_DECLARE(std::string, graphics_backend);
REXCVAR_DECLARE(bool, pdr_texture_noise_fix);
REXCVAR_DECLARE(int32_t, pdr_gpu_wait_mode);
REXCVAR_DECLARE(bool, dev_debug_runtime);
