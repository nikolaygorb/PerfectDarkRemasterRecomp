// pd_artifact_query_visible (0x82267468) is 4J's GetData on a light glare's occlusion query:
// 1 visible, 0 occluded, -1 not ready yet (-2 never issued). The glares read the queries of three
// frames ago and take "not ready" as hidden. On the 360 the GPU is never that far behind; here
// the host GPU can be, so with occlusion_query = async or strict (which never guess) a slow
// result hid every glare. A pending result is waited for, so the render thread stays at most
// three frames ahead of the host GPU. With the default fast mode results are always ready and
// nothing waits.
#include <chrono>
#include <iterator>
#include <thread>

#include <rex/hook.h>
#include <rex/logging.h>

#include "../game/functions.h"
#include "../game_cvars.h"

namespace
{
  constexpr int32_t kNotReady = -1;

  // After a wait runs out the GPU is far behind (loading, a stall); stop waiting for a while
  // instead of losing a full timeout on every glare.
  constexpr std::chrono::milliseconds kBackoff{250};
  constexpr std::chrono::seconds kStatsWindow{5};

  std::chrono::steady_clock::time_point g_no_wait_until;

  // Callers, by return address: each glare sample has a query from before the gun (the scene's
  // depth) and one from after it (the gun's own depth).
  struct CallSite
  {
    uint32_t lr;
    const char *name;
  };
  constexpr CallSite kCallSites[] = {
      {0x8215C1E0, "glare before gun"},
      {0x8215C1FC, "glare after gun"},
      {0x8223A274, "sun, first query"},
      {0x8223A290, "sun, second query"},
      {0x8224EC4C, "free, before-gun query"},
      {0x8224EC78, "free, after-gun query"},
  };
  constexpr size_t kOtherSite = std::size(kCallSites);

  // Final answers per caller, and the waits behind them. Nothing is logged in windows without
  // queries.
  class QueryStats
  {
  public:
    void Observe(uint32_t lr, int32_t result, bool waited, bool timed_out,
                 std::chrono::steady_clock::duration wait)
    {
      size_t site = 0;
      while (site < kOtherSite && kCallSites[site].lr != lr)
        ++site;
      Counts &counts = sites_[site];
      switch (result)
      {
      case 1:
        ++counts.visible;
        break;
      case 0:
        ++counts.occluded;
        break;
      case kNotReady:
        ++counts.not_ready;
        break;
      default:
        ++counts.other;
        break;
      }
      waits_ += waited;
      timeouts_ += timed_out;
      wait_total_ += wait;

      const auto now = std::chrono::steady_clock::now();
      if (now - window_started_ < kStatsWindow)
        return;
      REXLOG_INFO("Glare queries in {} s: {} waits ({} timed out), {:.1f} ms waiting", kStatsWindow.count(), waits_,
                  timeouts_, std::chrono::duration<double, std::milli>(wait_total_).count());
      for (size_t i = 0; i <= kOtherSite; ++i)
      {
        const Counts &c = sites_[i];
        if (c.visible || c.occluded || c.not_ready || c.other)
          REXLOG_INFO("  {}: {} visible, {} occluded, {} not ready, {} other",
                      i < kOtherSite ? kCallSites[i].name : "other caller", c.visible, c.occluded, c.not_ready,
                      c.other);
      }
      *this = {};
      window_started_ = now;
    }

  private:
    struct Counts
    {
      uint32_t visible = 0;
      uint32_t occluded = 0;
      uint32_t not_ready = 0;
      uint32_t other = 0;
    };

    Counts sites_[kOtherSite + 1];
    uint32_t waits_ = 0;
    uint32_t timeouts_ = 0;
    std::chrono::steady_clock::duration wait_total_{};
    std::chrono::steady_clock::time_point window_started_ = std::chrono::steady_clock::now();
  };

  QueryStats g_stats;

  // Asks again, yielding in between, until the result is ready or the time is up.
  bool WaitForResult(PPCContext &ctx, uint8_t *base, uint64_t pool, uint64_t query)
  {
    const auto now = std::chrono::steady_clock::now();
    if (now < g_no_wait_until)
      return false;
    const auto deadline = now + std::chrono::milliseconds(REXCVAR_GET(pdr_glare_query_wait_ms));
    do
    {
      std::this_thread::yield();
      ctx.r3.u64 = pool;
      ctx.r4.u64 = query;
      __imp__pd_artifact_query_visible(ctx, base);
      if (ctx.r3.s32 != kNotReady)
        return false;
    } while (std::chrono::steady_clock::now() < deadline);
    g_no_wait_until = std::chrono::steady_clock::now() + kBackoff;
    return true;
  }
} // namespace

REX_HOOK_RAW(pd_artifact_query_visible)
{
  const uint64_t pool = ctx.r3.u64;
  const uint64_t query = ctx.r4.u64;
  const uint32_t lr = static_cast<uint32_t>(ctx.lr);
  __imp__pd_artifact_query_visible(ctx, base);
  const bool wait = ctx.r3.s32 == kNotReady && REXCVAR_GET(pdr_glare_query_wait_ms) > 0;
  const bool log = REXCVAR_GET(pdr_log_glare_queries);
  if (!wait && !log)
    return;

  const auto start = std::chrono::steady_clock::now();
  const bool timed_out = wait && WaitForResult(ctx, base, pool, query);
  if (log)
    g_stats.Observe(lr, ctx.r3.s32, wait, timed_out, std::chrono::steady_clock::now() - start);
}
