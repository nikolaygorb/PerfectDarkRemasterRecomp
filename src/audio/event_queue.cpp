// An empty audio event queue no longer locks up the game (black screen when a
// mission is restarted).
//
// n_alEvtqNextEvent hands out the next event of a sequence/sound player and the
// microseconds until it. On an empty queue it returns an event of type -1 and a
// delay of 0, and the players' voice handlers loop "while (delay == 0)" - so they
// spin forever (Nintendo's own comment: the queue overflowed with events that
// don't post a next one; "the evtq should be increased"). 4J added an assert
// (twi) there. The handlers run on the sound thread inside its critical section,
// and the main thread waits for that section: black screen. Restarting a mission
// posts enough events at once to get there.
//
// Here an empty queue gives a short non-zero delay instead: the handler skips the
// type -1 event, returns, and looks at the queue again a moment later.
#include <rex/logging.h>

#include "../game/audio.h"
#include "../game/functions.h"
#include "../guest/memory.h"

namespace
{
  constexpr uint32_t kEmptyQueueDelayUs = 1000;
} // namespace

// n_alEvtqNextEvent(lock, evtq, evt) -> microseconds until evt. 4J's port passes
// the lock that stands in for the N64's interrupt mask first.
REX_HOOK_RAW(pd_naudio_evtq_next_event)
{
  const uint32_t evtq = ctx.r4.u32;
  const uint32_t evt = ctx.r5.u32;
  if (guest::Load32(base, evtq + pd::audio::kEvtqAllocListNext) != 0)
  {
    __imp__pd_naudio_evtq_next_event(ctx, base);
    return;
  }

  static bool logged = false;
  if (!logged)
  {
    logged = true;
    REXLOG_WARN("Audio event queue ran empty; continuing instead of spinning");
  }
  guest::Store16(base, evt + pd::audio::kEventType, pd::audio::kEventTypeNone);
  ctx.r3.u64 = kEmptyQueueDelayUs;
}
