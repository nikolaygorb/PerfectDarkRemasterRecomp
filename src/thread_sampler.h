#pragma once

namespace pdr
{
  // Samples the busiest threads of the process once, after pdr_sample_threads_after_s seconds,
  // and logs the functions they spend their time in. Does nothing unless that cvar is set.
  void StartThreadSampler();
} // namespace pdr
