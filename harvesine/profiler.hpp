#ifndef _PROFILER_HPP_
#define _PROFILER_HPP_

#include <stdio.h>

#include "types.h"
#include "timers.h"

#ifndef PROFILER
#define PROFILER 0
#endif

#define READ_BLOCK_TIMER read_os_timer

#ifndef READ_BLOCK_TIMER
#define READ_BLOCK_TIMER read_cpu_timer
#endif

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#if PROFILER

struct ProfileAnchor {
  u64 elapsed_exclusive; // does not include children
  u64 elapsed_inclusive; // does include children
  u64 hit_count;
  u64 processed_byte_count;
  const char* label;
};

static ProfileAnchor global_anchors[4096];
static u32 global_profiler_parent;

struct profile_block {
  profile_block(const char* label_, u32 index_, u64 byte_count) {
    parent = global_profiler_parent;

    index = index_;
    label = label_;

    ProfileAnchor *anchor = global_anchors + index;
    old_elapsed_inclusive = anchor->elapsed_inclusive;
    anchor->processed_byte_count += byte_count;

    global_profiler_parent = index_;

    start = READ_BLOCK_TIMER();
  }

  ~profile_block(void) {
    u64 elapsed = READ_BLOCK_TIMER() - start;
    global_profiler_parent = parent;

    ProfileAnchor *parent_anchor = global_anchors + parent;
    parent_anchor->elapsed_exclusive -= elapsed;

    ProfileAnchor *anchor = global_anchors + index;
    anchor->elapsed_exclusive += elapsed;
    anchor->elapsed_inclusive = old_elapsed_inclusive + elapsed;
    ++anchor->hit_count;

    anchor->label = label;
  }

  const char* label;
  u64 old_elapsed_inclusive;
  u64 start;
  u32 index;
  u32 parent;
};

#define NAME_CONCAT_NX(A, B) A##B
#define NAME_CONCAT(A, B) NAME_CONCAT_NX(A, B)
#define TIME_BANDWIDTH(Name, ByteCount) profile_block NAME_CONCAT(Block, __LINE__)(Name, __COUNTER__ + 1, ByteCount);

static void PrintTimeElapsed(u64 total_elapsed, ProfileAnchor *anchor) {
  f64 percent = 100.0 * ((f64)anchor->elapsed_exclusive / (f64)total_elapsed);

  printf("  %s[%lu]: %lu (%.2f%%", anchor->label, anchor->hit_count, anchor->elapsed_exclusive, percent);

  if (anchor->elapsed_inclusive != anchor->elapsed_exclusive) {
    f64 percent_with_children = 100.0 * ((f64)anchor->elapsed_inclusive / (f64)total_elapsed);
    printf(", %.2f%% with children", percent_with_children);
  }
  printf(")\n");

  // if (anchor->processed_byte_count) {
  //   f64 megabyte = 1024.0f * 1024.0f;
  //   f64 gigabyte = megabyte * 1024.0f;
  // 
  //   f64 seconds = (f64)anchor->elapsed_inclusive / (f64)TimerFreq;
  //   f64 BytesPerSecond = (f64)Anchor->ProcessedByteCount / Seconds;
  //   f64 Megabytes = (f64)Anchor->ProcessedByteCount / (f64)Megabyte;
  //   f64 GigabytesPerSecond = BytesPerSecond / Gigabyte;
  // 
  //   printf("  %.3fmb at %.2fgb/s", Megabytes, GigabytesPerSecond);
  // }
}

static void PrintAnchorData(u64 total_elapsed) {
  for (u32 index = 0; index < ARRAY_COUNT(global_anchors); ++index) {
    ProfileAnchor *anchor = global_anchors + index;
    if (anchor->elapsed_inclusive) {
      PrintTimeElapsed(total_elapsed, anchor);
    }
  }
}

#else

#define TIME_BANDWIDTH(...)
#define PrintAnchorData(...)

#endif // PROFILER

struct Profiler {
  u64 start;
  u64 end;
};

static Profiler global_profiler;

#define TIME_BLOCK(Name) TIME_BANDWIDTH(Name, 0)
#define TIME_FUNC TIME_BLOCK(__func__)

static inline uint64_t estimate_block_freq() {
  uint64_t milliseconds_to_wait = 100;
  uint64_t os_freq = read_os_timer_freq();

  uint64_t cpu_start = READ_BLOCK_TIMER();
  uint64_t os_start = read_os_timer();
  uint64_t os_end = 0;
  uint64_t os_elapsed = 0;
  uint64_t os_wait_time = os_freq * milliseconds_to_wait / 1000;

  while (os_elapsed < os_wait_time) {
    os_end = read_os_timer();
    os_elapsed = os_end - os_start;
  }

  uint64_t cpu_end = READ_BLOCK_TIMER();
  uint64_t cpu_elapsed = cpu_end - cpu_start;

  uint64_t cpu_freq = 0;
  if (os_elapsed) {
    cpu_freq = os_freq * cpu_elapsed / os_elapsed;
  }

  return cpu_freq;
}

static void BeginProfile(void) {
  global_profiler.start = READ_BLOCK_TIMER();
}

static void EndAndPrintProfile() {
  global_profiler.end = READ_BLOCK_TIMER();
  u64 cpu_freq = estimate_block_freq();

  u64 total_elapsed = global_profiler.end - global_profiler.start;

  if (cpu_freq > 0) {
    printf("\nTotal time: %0.4fms (CPU freq %lu)\n", 1000.0 * (f64)total_elapsed / (f64)cpu_freq, cpu_freq);
  }

  PrintAnchorData(total_elapsed);
}

#endif // _PROFILER_HPP_

