#include "graphics_backend.h"

#include <string>

#include <rex/cvar.h>
#include <rex/cvar_file.h>
#include <rex/logging.h>
#include <rex/system/gpu_plugin.h>

#include "../game_cvars.h"

namespace pdr
{
  namespace
  {
    // On Vulkan the light glares' occlusion tests against the scene's depth all fail (the
    // glares strobe or vanish) unless float24 depth is converted in the pixel shader, and then
    // the polygon offset has to be applied there too. On D3D12 the same settings make the gun
    // and hands sink into the floor and hide the glares, so Vulkan only. Changing the defaults
    // (not the values) keeps an explicit hardware.toml / --flag value in charge, and "Save to
    // config" doesn't write them, so a later D3D12 run doesn't pick them up.
    void ApplyVulkanDefaults()
    {
      if (!REXCVAR_GET(pdr_vulkan_float24_depth))
        return;
      rex::cvar::SetDefaultValue("depth_float24_convert_in_pixel_shader", "true");
      rex::cvar::SetDefaultValue("depth_float24_round", "true");
      rex::cvar::SetDefaultValue("depth_bias_shader_offset", "true");
    }
  } // namespace

  void LoadGpuPlugin(rex::RuntimeConfig &config)
  {
    if (config.gpu_plugin.empty())
      return;

    const std::string backend = REXCVAR_GET(graphics_backend);
    bool vulkan = false;
    if (backend != "any")
    {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin, backend);
      if (config.graphics)
        vulkan = backend == "vulkan";
      else
        REXLOG_WARN("graphics_backend '{}' unavailable, falling back to automatic selection",
                    backend);
    }
    if (!config.graphics)
    {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin);
#if !REX_HAS_D3D12
      // "any" takes D3D12 when it's built in (Windows), otherwise Vulkan (as plugin_main.cpp).
      vulkan = config.graphics != nullptr;
#endif
    }

    if (vulkan)
      ApplyVulkanDefaults();
  }
} // namespace pdr
