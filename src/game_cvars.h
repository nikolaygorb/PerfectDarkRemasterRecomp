// game_cvars.h - Game-specific CVAR declarations
#pragma once

#include <cstdint>
#include <string>

#include <rex/cvar.h>

REXCVAR_DECLARE(std::string, graphics_backend);
REXCVAR_DECLARE(bool, pdr_texture_noise_fix);
REXCVAR_DECLARE(bool, pdr_log_gpu_wait);
REXCVAR_DECLARE(int32_t, pdr_sample_threads_after_s);
REXCVAR_DECLARE(int32_t, pdr_sample_threads_duration_s);
REXCVAR_DECLARE(int32_t, pdr_gpu_wait_mode);
REXCVAR_DECLARE(bool, pdr_tp_enable);
REXCVAR_DECLARE(double, pdr_tp_offset_x);
REXCVAR_DECLARE(double, pdr_tp_offset_y);
REXCVAR_DECLARE(double, pdr_tp_offset_z);
REXCVAR_DECLARE(bool, pdr_skip_live_prompt);
REXCVAR_DECLARE(bool, dev_debug_runtime);
