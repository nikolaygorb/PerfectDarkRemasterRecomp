#pragma once

#include <rex/runtime.h>

namespace pdr
{
  // Loads config.gpu_plugin with the graphics_backend cvar's backend (falling back to "any").
  void LoadGpuPlugin(rex::RuntimeConfig &config);
} // namespace pdr
