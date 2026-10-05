#include "thread_sampler.h"

#ifdef _WIN32

#include <algorithm>
#include <chrono>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <windows.h>
#include <tlhelp32.h>

#include <dbghelp.h>

#include <fmt/format.h>
#include <rex/logging.h>

#include "game_cvars.h"

#pragma comment(lib, "dbghelp.lib")

namespace pdr
{
  namespace
  {
    using Clock = std::chrono::steady_clock;

    constexpr int kBusyProbeSeconds = 2;
    constexpr double kBusyThreshold = 0.5; // share of one core
    constexpr size_t kTopFunctions = 12;
    constexpr size_t kMaxSampledThreads = 6;
    constexpr size_t kStackBytes = 3072;
    constexpr size_t kStackWords = kStackBytes / sizeof(DWORD64);

    struct Sample
    {
      DWORD64 rip;
      uint32_t stack_bytes;
      DWORD64 stack[kStackWords];
    };

    struct SampledThread
    {
      DWORD id;
      HANDLE handle;
      std::vector<Sample> samples;
    };

    struct ImageRange
    {
      DWORD64 begin;
      DWORD64 end;
    };

    ImageRange GetImageRange()
    {
      const auto base = reinterpret_cast<DWORD64>(GetModuleHandleW(nullptr));
      const auto *dos = reinterpret_cast<const IMAGE_DOS_HEADER *>(base);
      const auto *nt = reinterpret_cast<const IMAGE_NT_HEADERS64 *>(base + dos->e_lfanew);
      return {base, base + nt->OptionalHeader.SizeOfImage};
    }

    uint64_t CpuTime(HANDLE thread)
    {
      FILETIME creation, exit, kernel, user;
      if (!GetThreadTimes(thread, &creation, &exit, &kernel, &user))
        return 0;
      auto to_ticks = [](const FILETIME &time) { return (uint64_t(time.dwHighDateTime) << 32) | time.dwLowDateTime; };
      return to_ticks(kernel) + to_ticks(user); // 100 ns ticks
    }

    std::vector<SampledThread> OpenOwnThreads()
    {
      std::vector<SampledThread> threads;
      HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
      if (snapshot == INVALID_HANDLE_VALUE)
        return threads;
      THREADENTRY32 entry{sizeof(entry)};
      for (BOOL more = Thread32First(snapshot, &entry); more; more = Thread32Next(snapshot, &entry))
      {
        if (entry.th32OwnerProcessID != GetCurrentProcessId() || entry.th32ThreadID == GetCurrentThreadId())
          continue;
        HANDLE handle = OpenThread(THREAD_QUERY_LIMITED_INFORMATION | THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT,
                                   FALSE, entry.th32ThreadID);
        if (handle)
          threads.push_back({entry.th32ThreadID, handle, {}});
      }
      CloseHandle(snapshot);
      return threads;
    }

    std::vector<SampledThread> KeepBusyThreads(std::vector<SampledThread> threads)
    {
      std::vector<uint64_t> before;
      for (const SampledThread &thread : threads)
        before.push_back(CpuTime(thread.handle));
      std::this_thread::sleep_for(std::chrono::seconds(kBusyProbeSeconds));

      std::vector<std::pair<double, size_t>> busy;
      for (size_t i = 0; i < threads.size(); ++i)
      {
        const double share = double(CpuTime(threads[i].handle) - before[i]) / (kBusyProbeSeconds * 1e7);
        if (share >= kBusyThreshold)
          busy.emplace_back(share, i);
      }
      std::sort(busy.rbegin(), busy.rend());

      std::vector<SampledThread> kept;
      for (size_t i = 0; i < busy.size() && i < kMaxSampledThreads; ++i)
      {
        SampledThread &busy_thread = threads[busy[i].second];
        kept.push_back(busy_thread);
        busy_thread.handle = nullptr;
      }
      for (const SampledThread &thread : threads)
        if (thread.handle)
          CloseHandle(thread.handle);
      return kept;
    }

    // Nothing may allocate while a thread is suspended, hence the reserved sample buffers.
    void CollectSamples(std::vector<SampledThread> &threads, int seconds)
    {
      for (SampledThread &thread : threads)
        thread.samples.reserve(size_t(seconds) * 700);
      const auto end = Clock::now() + std::chrono::seconds(seconds);
      while (Clock::now() < end)
      {
        for (SampledThread &thread : threads)
        {
          if (SuspendThread(thread.handle) == DWORD(-1))
            continue;
          CONTEXT context{};
          context.ContextFlags = CONTEXT_CONTROL;
          const bool captured = GetThreadContext(thread.handle, &context);
          Sample *sample = captured && thread.samples.size() < thread.samples.capacity()
                               ? &thread.samples.emplace_back()
                               : nullptr;
          if (sample)
          {
            SIZE_T read = 0;
            sample->rip = context.Rip;
            ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void *>(context.Rsp), sample->stack,
                              kStackBytes, &read);
            sample->stack_bytes = uint32_t(read);
          }
          ResumeThread(thread.handle);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }

    void LogRanking(const char *title, const std::unordered_map<std::string, size_t> &counts, size_t samples)
    {
      std::vector<std::pair<std::string, size_t>> ranked(counts.begin(), counts.end());
      std::sort(ranked.begin(), ranked.end(), [](const auto &a, const auto &b) { return a.second > b.second; });
      REXLOG_INFO("  {}:", title);
      for (size_t i = 0; i < ranked.size() && i < kTopFunctions; ++i)
        REXLOG_INFO("    {:5.1f}% {}", 100.0 * double(ranked[i].second) / double(samples), ranked[i].first);
    }

    void LogProfile(const std::vector<SampledThread> &threads)
    {
      HANDLE process = GetCurrentProcess();
      SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);
      if (!SymInitialize(process, nullptr, TRUE))
      {
        REXLOG_WARN("Thread sampler: SymInitialize failed");
        return;
      }
      alignas(SYMBOL_INFO) char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
      auto *symbol = reinterpret_cast<SYMBOL_INFO *>(buffer);
      symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
      symbol->MaxNameLen = MAX_SYM_NAME;

      const ImageRange image = GetImageRange();
      std::unordered_map<DWORD64, std::string> names;
      auto name_of = [&](DWORD64 address) -> const std::string & {
        auto [entry, created] = names.try_emplace(address);
        if (created)
        {
          DWORD64 displacement = 0;
          entry->second = SymFromAddr(process, address, &displacement, symbol) ? symbol->Name
                                                                              : fmt::format("0x{:X}", address);
        }
        return entry->second;
      };

      for (const SampledThread &thread : threads)
      {
        std::unordered_map<std::string, size_t> self_counts;
        std::unordered_map<std::string, size_t> stack_counts;
        for (const Sample &sample : thread.samples)
        {
          ++self_counts[name_of(sample.rip)];
          // Conservative scan: any stack word pointing into the image may be a return address.
          std::unordered_set<std::string> on_stack;
          for (size_t i = 0; i < sample.stack_bytes / sizeof(DWORD64); ++i)
            if (sample.stack[i] >= image.begin && sample.stack[i] < image.end)
            {
              const std::string &name = name_of(sample.stack[i]);
              // Recompiled guest functions: sub_XXXXXXXX, or pd_<name> once named
              // (function_names.toml); __imp__ variants contain the same.
              if (name.find("sub_") != std::string::npos || name.find("pd_") != std::string::npos)
                on_stack.insert(name);
            }
          for (const std::string &name : on_stack)
            ++stack_counts[name];
        }

        REXLOG_INFO("Thread sampler: thread {}, {} samples", thread.id, thread.samples.size());
        LogRanking("running in", self_counts, thread.samples.size());
        LogRanking("guest function on the stack", stack_counts, thread.samples.size());
      }
      SymCleanup(process);
    }

    void Run(int after_seconds, int duration_seconds)
    {
      std::this_thread::sleep_for(std::chrono::seconds(after_seconds));
      std::vector<SampledThread> threads = KeepBusyThreads(OpenOwnThreads());
      REXLOG_INFO("Thread sampler: sampling {} busy threads for {} s", threads.size(), duration_seconds);
      CollectSamples(threads, duration_seconds);
      LogProfile(threads);
      for (const SampledThread &thread : threads)
        CloseHandle(thread.handle);
    }
  } // namespace

  void StartThreadSampler()
  {
    const int after_seconds = REXCVAR_GET(pdr_sample_threads_after_s);
    if (after_seconds > 0)
      std::thread(Run, after_seconds, REXCVAR_GET(pdr_sample_threads_duration_s)).detach();
  }
} // namespace pdr

#else

namespace pdr
{
  void StartThreadSampler() {}
} // namespace pdr

#endif
