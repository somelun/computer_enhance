#ifndef _PROFILER_HPP_
#define _PROFILER_HPP_

#include "types.h"
#include "timers.h"

#include <stdio.h>

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

struct ProfileAnchor {
  u64 elapsed;
  u64 elapsed_children;
  u64 hit_count;
  const char* label;
};

struct Profiler {
  ProfileAnchor anchors[4096];

  u64 start;
  u64 end;
};

static Profiler global_profiler;
static u32 global_profiler_parent;

struct profile_block {
  profile_block(const char* label_, u32 index_) {
    parent = global_profiler_parent;

    index = index_;
    label = label_;

    global_profiler_parent = index_;

    start = read_cpu_timer();
  }

  ~profile_block(void) {
    u64 elapsed = read_cpu_timer() - start;
    global_profiler_parent = parent;

    ProfileAnchor *parent_anchor = global_profiler.anchors + parent;
    parent_anchor->elapsed_children += elapsed;

    ProfileAnchor *anchor = global_profiler.anchors + index;
    anchor->elapsed += elapsed;
    ++anchor->hit_count;

    anchor->label = label;
  }

  const char* label;
  u64 start;
  u32 index;
  u32 parent;
};

#define NAME_CONCAT_NX(A, B) A##B
#define NAME_CONCAT(A, B) NAME_CONCAT_NX(A, B)
#define TIME_BLOCK(Name) profile_block NAME_CONCAT(Block, __LINE__)(Name, __COUNTER__ + 1);
#define TimeFunction TIME_BLOCK(__func__)

static void PrintTimeElapsed(u64 total_elapsed, ProfileAnchor *anchor) {
  u64 elapsed = anchor->elapsed - anchor->elapsed_children;
  f64 percent = 100.0 * ((f64)elapsed / (f64)total_elapsed);

  printf("  %s[%lu]: %lu (%.2f%%", anchor->label, anchor->hit_count, elapsed, percent);

  if (anchor->elapsed_children) {
    f64 percent_with_children = 100.0 * ((f64)anchor->elapsed / (f64)total_elapsed);
    printf(", %.2f%% with children", percent_with_children);
  }
  printf(")\n");
}

static void BeginProfile(void) {
  global_profiler.start = read_cpu_timer();
}

static void EndAndPrintProfile() {
  global_profiler.end = read_cpu_timer();
  u64 cpu_freq = get_cpu_freq();

  u64 TotalCPUElapsed = global_profiler.end - global_profiler.start;

  if (cpu_freq > 0) {
    printf("\nTotal time: %0.4fms (CPU freq %lu)\n", 1000.0 * (f64)TotalCPUElapsed / (f64)cpu_freq, cpu_freq);
  }

  for (u32 AnchorIndex = 0; AnchorIndex < ARRAY_COUNT(global_profiler.anchors); ++AnchorIndex) {
    ProfileAnchor *Anchor = global_profiler.anchors + AnchorIndex;
    if (Anchor->elapsed) {
      PrintTimeElapsed(TotalCPUElapsed, Anchor);
    }
  }
}

#endif // _PROFILER_HPP_

