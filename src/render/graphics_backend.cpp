#include "graphics_backend.h"

#include <string>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/system/gpu_plugin.h>

#include "../game_cvars.h"

namespace pdr
{
  void LoadGpuPlugin(rex::RuntimeConfig &config)
  {
    if (config.gpu_plugin.empty())
      return;

    const std::string backend = REXCVAR_GET(graphics_backend);
    if (backend != "any")
    {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin, backend);
      if (!config.graphics)
        REXLOG_WARN("graphics_backend '{}' unavailable, falling back to automatic selection",
                    backend);
    }
    if (!config.graphics)
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin);
  }
} // namespace pdr
