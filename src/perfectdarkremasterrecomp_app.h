// perfectdarkremasterrecomp - ReXGlue Recompiled Project
//
// Customize your app by overriding virtual hooks from rex::ReXApp.

#pragma once

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <mutex>
#include <unordered_map>

#include <rex/cvar.h>
#include <rex/filesystem.h>
#include <rex/logging.h>
#include <rex/memory/utils.h>
#include <rex/rex_app.h>
#include <rex/runtime.h>
#include <rex/system/function_dispatcher.h>
#include <rex/system/gpu_plugin.h>

namespace GameConstants
{
  constexpr uint32_t kCodeBase = 0x820C0000;
  constexpr uint32_t kCodeEnd = 0x825BB934;
}

REXCVAR_DEFINE_STRING(graphics_backend, "any", "GPU",
                      "Graphics API backend: any, d3d12, vulkan")
    .allowed({"any", "d3d12", "vulkan"});

// Experimental port of the community xenia-canary "Fix texture noise on
// NVIDIA GPUs" patch for Perfect Dark (584109C2). Off by default to match
// upstream.
REXCVAR_DEFINE_BOOL(pdr_texture_noise_fix, false, "Gameplay",
                    "Experimental: fix texture noise on NVIDIA GPUs (ported "
                    "from xenia-canary game-patches)");

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
  }

  // void OnLoadXexImage(std::string& xex_image) override {}
  void OnPostLoadXexImage() override
  {
    if (!REXCVAR_GET(pdr_texture_noise_fix))
    {
      return;
    }
    auto *rt = rex::Runtime::instance();
    uint8_t *base = rt ? rt->virtual_membase() : nullptr;
    if (!base)
    {
      return;
    }
    constexpr uint32_t kTextureNoiseAddr = 0x82014b7b;
    constexpr uint8_t kTextureNoiseValue = 0x30;
    uint8_t *p = base + kTextureNoiseAddr;
    // The byte lives in a read-only-mapped section (write access violation
    // without this) - unprotect just long enough to patch it, then restore
    // whatever access it had before.
    rex::memory::PageAccess old_access{};
    rex::memory::Protect(p, 1, rex::memory::PageAccess::kReadWrite, &old_access);
    p[0] = kTextureNoiseValue;
    rex::memory::Protect(p, 1, old_access, nullptr);
  }

  void OnPostSetup() override
  {
    LoadSettingsFiles();

    if (const char *e = std::getenv("NO_STUB_SWEEP"); e && *e == '1')
    {
      return;
    }
    PerformMissingFunctionScan();
    PerformStubSweep();
  }

  void OnConfigurePaths(rex::PathConfig &paths) override
  {
    // --game_data_root / REX_GAME_DATA_ROOT still win: this cvar is read
    // before any config file loads, so it's already non-empty here if set.
    if (paths.game_data_root.empty())
    {
      paths.game_data_root = RepoRoot() / "assets";
    }

    // Hardware/rendering settings become the SDK's primary config file, so
    // the in-game Settings overlay's "Save to config" also lands here.
    paths.config_path = SettingsDir() / "hardware.toml";

    // Input mapping lives in its own file, loaded manually below (the SDK
    // only auto-loads the single path above).
    LoadSettingsFiles();
  }

private:
  static std::filesystem::path RepoRoot()
  {
    namespace fs = std::filesystem;
    fs::path dir = rex::filesystem::GetExecutableFolder();
    for (int i = 0; i < 6 && !dir.empty() && dir.has_parent_path(); ++i)
    {
      if (fs::exists(dir / "fable2recomp_manifest.toml") || fs::exists(dir / "assets"))
      {
        break;
      }
      dir = dir.parent_path();
    }
    return dir;
  }

  static std::filesystem::path SettingsDir() { return RepoRoot() / "settings"; }

  static void LoadSettingsFiles()
  {
    namespace fs = std::filesystem;
    fs::path dir = SettingsDir();
    for (const char *file : {"hardware.toml", "mapping.toml"})
    {
      if (fs::path p = dir / file; fs::exists(p))
      {
        rex::cvar::LoadConfig(p);
      }
    }
  }

  void PerformStubSweep()
  {
    auto *rt = rex::Runtime::instance();
    auto *fd = rt ? rt->function_dispatcher() : nullptr;
    uint8_t *base = rt ? rt->virtual_membase() : nullptr;
    if (!fd || !base)
    {
      return;
    }

    static FILE *stub_log = std::fopen("logs/stub_sweep.txt", "w");
    static std::mutex stub_mutex;
    static std::unordered_map<uint32_t, uint32_t> stub_hits;

    static PPCFunc *stub = [](PPCContext &ctx, uint8_t *) noexcept
    {
      uint32_t addr = ctx.ctr.u32;
      uint32_t lr = ctx.lr;
      std::lock_guard<std::mutex> lock(stub_mutex);
      uint32_t &count = stub_hits[addr];
      if (count == 0 && stub_log)
      {
        std::fprintf(stub_log,
                     "[stub] addr=0x%08X LR=0x%08X r3=0x%08X r4=0x%08X r5=0x%08X r6=0x%08X\n",
                     addr, lr, ctx.r3.u32, ctx.r4.u32, ctx.r5.u32, ctx.r6.u32);
        std::fflush(stub_log);
      }
      ++count;
    };

    uint32_t stubbed = 0;
    for (uint32_t addr = GameConstants::kCodeBase; addr < GameConstants::kCodeEnd; addr += 4)
    {
      if (!fd->GetFunction(addr))
      {
        fd->SetFunction(addr, stub);
        ++stubbed;
      }
    }
    if (stub_log)
    {
      std::fprintf(stub_log, "=== stub sweep: scanned %u addresses, stubbed %u ===\n",
                   (GameConstants::kCodeEnd - GameConstants::kCodeBase) / 4, stubbed);
      std::fflush(stub_log);
    }
  }

  void PerformMissingFunctionScan()
  {
    auto *rt = rex::Runtime::instance();
    auto *fd = rt ? rt->function_dispatcher() : nullptr;

    if (!fd)
    {
      return;
    }

    FILE *log = std::fopen("logs/missed_functions.txt", "w");
    if (!log)
    {
      return;
    }

    uint32_t missing = 0;
    uint32_t registered = 0;

    for (uint32_t addr = GameConstants::kCodeBase;
         addr < GameConstants::kCodeEnd;
         addr += 4)
    {

      if (fd->GetFunction(addr))
      {
        ++registered;
      }
      else
      {
        ++missing;

        std::fprintf(log,
                     "[missed] addr=0x%08X\n",
                     addr);
      }
    }

    std::fprintf(log,
                 "=== scan: scanned=%u registered=%u missed=%u ===\n",
                 (GameConstants::kCodeEnd -
                  GameConstants::kCodeBase) /
                     4,
                 registered,
                 missing);

    std::fclose(log);
  }

  // Override virtual hooks for customization:
  // void OnPostInitLogging() override {}
  // void OnPreSetup(rex::RuntimeConfig& config) override {}
  // void OnLoadXexImage(std::string& xex_image) override {}
  // void OnPostLoadXexImage() override {}
  // void OnPostSetup() override {}
  // void OnCreateDialogs(rex::ui::ImGuiDrawer* drawer) override {}
  // std::unique_ptr<rex::ui::ImGuiDialog> CreateAchievementsOverlay() override;
  // std::unique_ptr<rex::ui::AchievementNotificationDialog>
  // CreateAchievementNotificationDialog() override;
  // void OnShutdown() override {}
  // void OnConfigurePaths(rex::PathConfig& paths) override {}
};
