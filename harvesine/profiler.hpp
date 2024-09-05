#ifndef _PROFILER_HPP_
#define _PROFILER_HPP_

#include <stdio.h>

#include "types.h"
#include "timers.h"

#ifndef PROFILER
#define PROFILER 0
#endif

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
  u32 parent;
};

static ProfileAnchor global_anchors[4096];
static u32 global_profiler_parent;

struct profile_block {
  profile_block(const char* label_, u32 index_, u64 byte_count) {
    parent = global_profiler_parent;

    index = index_;
    label = label_;

    ProfileAnchor *anchor = global_anchors + index;
    anchor->parent = parent;
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

static void PrintTimeElapsed(u64 total_elapsed, u64 timer_freq, ProfileAnchor *anchor) {
  f64 percent = 100.0 * ((f64)anchor->elapsed_exclusive / (f64)total_elapsed);

  if (anchor->parent > 0) {
    printf("  ");
  }
  printf("  %s[%lu]: %lu (%.2f%%", anchor->label, anchor->hit_count, anchor->elapsed_exclusive, percent);

  if (anchor->elapsed_inclusive != anchor->elapsed_exclusive) {
    f64 percent_with_children = 100.0 * ((f64)anchor->elapsed_inclusive / (f64)total_elapsed);
    printf(", %.2f%% with children", percent_with_children);
  }

  if (anchor->processed_byte_count) {
    f64 megabyte = 1024.0f * 1024.0f;
    f64 gigabyte = megabyte * 1024.0f;

    f64 seconds = (f64)anchor->elapsed_inclusive / (f64)timer_freq;
    f64 bytes_per_second = (f64)anchor->processed_byte_count / seconds;
    f64 megabytes = (f64)anchor->processed_byte_count / (f64)megabyte;
    f64 gigabytes_per_second = bytes_per_second / gigabyte;

    printf("  %.3fmb at %.2fgb/s", megabytes, gigabytes_per_second);
  }

  printf(")\n");
}

static void PrintAnchorData(u64 total_elapsed, u64 timer_freq) {
  for (u32 index = 0; index < ARRAY_COUNT(global_anchors); ++index) {
    ProfileAnchor *anchor = global_anchors + index;
    if (anchor->elapsed_inclusive) {
      PrintTimeElapsed(total_elapsed, timer_freq, anchor);
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


static void BeginProfile(void) {
  global_profiler.start = READ_BLOCK_TIMER();
}

static void EndAndPrintProfile() {
  global_profiler.end = READ_BLOCK_TIMER();
  u64 timer_freq = read_cpu_timer_freq();

  u64 total_elapsed = global_profiler.end - global_profiler.start;

  if (timer_freq > 0) {
    printf("\nTotal time: %0.4fms (Timer freq %lu)\n", 1000.0 * (f64)total_elapsed / (f64)timer_freq, timer_freq);
  }

  PrintAnchorData(total_elapsed, timer_freq);
}

#endif // _PROFILER_HPP_

