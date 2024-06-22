#ifndef _PROFILER_HPP_
#define _PROFILER_HPP_

#include <stdio.h>

#include "types.h"
#include "timers.h"

#ifndef PROFILER
#define PROFILER 0
#endif

#define ARRAY_COUNT(Array) (sizeof(Array) / sizeof((Array)[0]))

#if PROFILER

struct ProfileAnchor {
  u64 elapsed_exclusive; // does not include children
  u64 elapsed_inclusive; // does include children
  u64 hit_count;
  const char* label;
};

static ProfileAnchor global_anchors[4096];
static u32 global_profiler_parent;

struct profile_block {
  profile_block(const char* label_, u32 index_) {
    parent = global_profiler_parent;

    index = index_;
    label = label_;

    ProfileAnchor *anchor = global_anchors + index;
    old_elapsed_inclusive = anchor->elapsed_inclusive;

    global_profiler_parent = index_;

    start = read_cpu_timer();
  }

  ~profile_block(void) {
    u64 elapsed = read_cpu_timer() - start;
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
#define TIME_BLOCK(Name) profile_block NAME_CONCAT(Block, __LINE__)(Name, __COUNTER__ + 1);

static void PrintTimeElapsed(u64 total_elapsed, ProfileAnchor *anchor) {
  f64 percent = 100.0 * ((f64)anchor->elapsed_exclusive / (f64)total_elapsed);

  printf("  %s[%lu]: %lu (%.2f%%", anchor->label, anchor->hit_count, anchor->elapsed_exclusive, percent);

  if (anchor->elapsed_inclusive != anchor->elapsed_exclusive) {
    f64 percent_with_children = 100.0 * ((f64)anchor->elapsed_inclusive / (f64)total_elapsed);
    printf(", %.2f%% with children", percent_with_children);
  }
  printf(")\n");
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

#define TIME_BLOCK(...)
#define PrintAnchorData(...)

#endif // PROFILER

struct Profiler {
  u64 start;
  u64 end;
};

static Profiler global_profiler;

#define TIME_FUNC TIME_BLOCK(__func__)


static void BeginProfile(void) {
  global_profiler.start = read_cpu_timer();
}

static void EndAndPrintProfile() {
  global_profiler.end = read_cpu_timer();
  u64 cpu_freq = get_cpu_freq();

  u64 total_elapsed = global_profiler.end - global_profiler.start;

  if (cpu_freq > 0) {
    printf("\nTotal time: %0.4fms (CPU freq %lu)\n", 1000.0 * (f64)total_elapsed / (f64)cpu_freq, cpu_freq);
  }

  PrintAnchorData(total_elapsed);
}

#endif // _PROFILER_HPP_

