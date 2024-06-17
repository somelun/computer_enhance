#ifndef PROFILER_HPP_
#define PROFILER_HPP_

#include "types.h"
#include "timers.h"

#include <stdio.h>

// struct profiler_entry {
//   profiler_entry() {
//     start = read_cpu_timer();
//   }
// 
//   ~profiler_entry() {
//     end = read_cpu_timer();
//   }
// 
//   uint16_t start;
//   uint16_t end;
//   const char* label;
// };
// 
// struct profiler {
//   struct profiler_entry entries[128];
// 
// };
// 
// static struct profiler global_profiler;

// #define 
//

#define ArrayCount(Array) (sizeof(Array)/sizeof((Array)[0]))

struct profile_anchor {
  u64 elapsed;
  u64 hit_count;
  const char* label;
};

struct profiler {
  profile_anchor anchors[4096];

  u64 start;
  u64 end;
};
static profiler global_profiler;

struct profile_block {
  profile_block(const char* label_, u32 index_) {
    index = index_;
    label = label_;
    start = read_cpu_timer();
  }

  ~profile_block(void) {
    u64 elapsed = read_cpu_timer() - start;

    profile_anchor *anchor = global_profiler.anchors + index;
    anchor->elapsed += elapsed;
    ++anchor->hit_count;

    anchor->label = label;
  }

  const char* label;
  u64 start;
  u32 index;
};

#define NameConcat2(A, B) A##B
#define NameConcat(A, B) NameConcat2(A, B)
#define TimeBlock(Name) profile_block NameConcat(Block, __LINE__)(Name, __COUNTER__ + 1);
#define TimeFunction TimeBlock(__func__)

static void PrintTimeElapsed(u64 total_elapsed, profile_anchor *anchor) {
  u64 elapsed = anchor->elapsed;
  f64 percent = 100.0 * ((f64)elapsed / (f64)total_elapsed);
  printf("  %s[%lu]: %lu (%.2f%%)\n", anchor->label, anchor->hit_count, elapsed, percent);
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

  for (u32 AnchorIndex = 0; AnchorIndex < ArrayCount(global_profiler.anchors); ++AnchorIndex) {
    profile_anchor *Anchor = global_profiler.anchors + AnchorIndex;
    if (Anchor->elapsed) {
      PrintTimeElapsed(TotalCPUElapsed, Anchor);
    }
  }
}

#endif

