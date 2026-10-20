// perfectdarkremasterrecomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cstdlib>

#include <rex/cvar.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/system/gpu_plugin.h>

#include "debug_tools.h"
#include "game_cvars.h"
#include "game_patches.h"
#include "mod/mouse_look/mouse_look.h"
#include "mod/third_person/third_person.h"
#include "thread_sampler.h"
#include "utils.h"

#ifdef REXGLUE_ENABLE_PERF_COUNTERS
#include <rex/perf/counter.h>
#endif

class PerfectdarkremasterrecompApp : public rex::ReXApp
{
public:
  using rex::ReXApp::ReXApp;

  static std::unique_ptr<rex::ui::WindowedApp> Create(
      rex::ui::WindowedAppContext &ctx)
  {
    return std::unique_ptr<PerfectdarkremasterrecompApp>(new PerfectdarkremasterrecompApp(ctx, "perfectdarkremasterrecomp",
                                                                                          PPCImageConfig));
  }

  void OnPreSetup(rex::RuntimeConfig &config) override
  {
    std::string backend = REXCVAR_GET(graphics_backend);
    if (backend != "any" && !config.gpu_plugin.empty())
    {
      config.graphics = rex::system::LoadGpuPlugin(config.gpu_plugin, backend);
      if (!config.graphics)
      {
        REXLOG_WARN("graphics_backend '{}' unavailable, falling back to automatic selection",
                    backend);
      }
    }

#ifdef _WIN32
    // "any" takes D3D12 first on Windows.
    const bool vulkan = backend == "vulkan" && config.graphics;
#else
    const bool vulkan = backend == "any" || (backend == "vulkan" && config.graphics);
#endif
    if (vulkan && REXCVAR_GET(pdr_vulkan_float24_depth))
    {
      // On Vulkan the light glares' occlusion tests against the scene's depth all fail (the
      // glares strobe or vanish) unless float24 depth is converted in the pixel shader, and
      // then the polygon offset has to be applied there too. On D3D12 the same settings make
      // the gun and hands sink into the floor and hide the glares, so Vulkan only.
      rex::cvar::SetFlagByName("depth_float24_convert_in_pixel_shader", "true");
      rex::cvar::SetFlagByName("depth_float24_round", "true");
      rex::cvar::SetFlagByName("depth_bias_shader_offset", "true");
    }
  }

  // void OnLoadXexImage(std::string& xex_image) override {}
  void OnPostLoadXexImage() override
  {
    game_patches::ApplyTextureNoiseFix();
  }

  void OnCreateDialogs(rex::ui::ImGuiDrawer *drawer) override
  {
    // Registered before OnPostSetup reloads the settings files, so the key
    // can be rebound from mapping.toml.
    third_person::RegisterBinds();

    // Window exists from this point on. Native mouse look (pdr_mouse_look) only
    // takes the mouse over while the hook in src/mod/mouse_look/hook.cpp runs.
    mouse_look::MouseLook::Get().Attach(window(), drawer);
  }

  void OnPostSetup() override
  {
    utils::LoadSettingsFiles();
    pdr::StartThreadSampler();

#ifdef REXGLUE_ENABLE_PERF_COUNTERS
    // SDK exposes the perf_log_csv cvar but never wires it up; do it here so
    // --perf_log_csv=<path> actually produces per-frame CSV output.
    // Use GetFlagByName to avoid needing a cross-DLL declaration of the cvar.
    rex::perf::SetCsvLogPath(rex::cvar::GetFlagByName("perf_log_csv"));
#endif

    // Run debug tools only if enabled via hardware.toml (dev_debug_runtime = true)
    if (REXCVAR_GET(dev_debug_runtime))
    {
      debug_tools::PerformMissingFunctionScan();
      debug_tools::PerformStubSweep();
    }
  }

  void OnShutdown() override
  {
#ifdef REXGLUE_ENABLE_PERF_COUNTERS
    rex::perf::FlushCsv();
#endif
  }

  void OnConfigurePaths(rex::PathConfig &paths) override
  {
    // --game_data_root / REX_GAME_DATA_ROOT still win: this cvar is read
    // before any config file loads, so it's already non-empty here if set.
    if (paths.game_data_root.empty())
    {
      paths.game_data_root = utils::RepoRoot() / "assets";
    }

    // Hardware/rendering settings become the SDK's primary config file, so
    // the in-game Settings overlay's "Save to config" also lands here.
    paths.config_path = utils::SettingsDir() / "hardware.toml";

    // Input mapping lives in its own file, loaded manually below (the SDK
    // only auto-loads the single path above).
    utils::LoadSettingsFiles();
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
