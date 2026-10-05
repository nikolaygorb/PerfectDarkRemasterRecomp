// pd_xdk_ring_space_poll (0x82329E08) is the XDK poll behind BlockUntilRingSpace: it returns nonzero to keep
// waiting for the GPU to free command-ring space and zero once the wait is over. The
// original poll is a few nops, so the render thread spins flat out while the emulated
// GPU is behind. Only the pause between polls changes; the result is untouched.
#include <chrono>
#include <thread>

#include <rex/hook.h>
#include <rex/logging.h>

#include "../game/functions.h"
#include "../game_cvars.h"

namespace
{
  enum GpuWaitMode : int32_t
  {
    kBusySpin = 0,
    kYield = 1,
    kSleep = 2,
  };

  constexpr std::chrono::microseconds kSleepPause{200};
  constexpr std::chrono::seconds kStatsWindow{5};

  // The poll is only called while the ring has too little space, so calls close together form
  // one wait. Nothing is logged in windows without waits.
  class RingWaitStats
  {
  public:
    void Observe()
    {
      const auto now = std::chrono::steady_clock::now();
      if (waiting_ && now - last_poll_ < kWaitGap)
        total_ += now - last_poll_;
      else
        ++waits_;
      waiting_ = true;
      last_poll_ = now;
      if (now - window_started_ >= kStatsWindow)
      {
        REXLOG_INFO("Render thread waited for command ring space {} times, {:.1f} ms in {} s", waits_,
                    std::chrono::duration<double, std::milli>(total_).count(), kStatsWindow.count());
        waits_ = 0;
        total_ = {};
        window_started_ = now;
      }
    }

  private:
    static constexpr std::chrono::milliseconds kWaitGap{2};

    bool waiting_ = false;
    uint32_t waits_ = 0;
    std::chrono::steady_clock::duration total_{};
    std::chrono::steady_clock::time_point last_poll_;
    std::chrono::steady_clock::time_point window_started_ = std::chrono::steady_clock::now();
  };
} // namespace

REX_HOOK_RAW(pd_xdk_ring_space_poll)
{
  __imp__pd_xdk_ring_space_poll(ctx, base);
  if (REXCVAR_GET(pdr_log_gpu_wait))
  {
    thread_local RingWaitStats stats;
    stats.Observe();
  }
  if (ctx.r3.u32 == 0)
    return;

  switch (REXCVAR_GET(pdr_gpu_wait_mode))
  {
  case kYield:
    std::this_thread::yield();
    break;
  case kSleep:
    std::this_thread::sleep_for(kSleepPause);
    break;
  default:
    break;
  }
}
