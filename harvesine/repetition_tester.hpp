#ifndef _RepTester_HPP_
#define _RepTester_HPP_

#include <stdio.h>

#include "types.h"
#include "timers.h"

enum class RepTestMode : u8 {
  Uninitialized,
  Testing,
  Completed,
  Error,
};

struct RepTestResults {
  u64 test_count;
  u64 total_time;
  u64 max_time;
  u64 min_time;
};

struct RepTester {
  u64 target_processed_byte_count;
  u64 cpu_timer_freq;
  u64 try_for_time;
  u64 tests_started_at;

  RepTestMode test_mode;
  bool print_new_minimums;
  u32 open_block_count;
  u32 close_block_count;
  u64 time_accumulated_on_this_test;
  u64 bytes_accumulated_on_this_test;

  RepTestResults results;
};

static f64 seconds_from_cpu_time(f64 cpu_time, u64 cpu_timer_freq) {
  f64 result = 0.0;
  if (cpu_timer_freq) {
    result = (cpu_time / (f64)cpu_timer_freq);
  }

  return result;
}

static void print_time(char const *label, f64 cpu_time, u64 cpu_timer_freq, u64 byte_count) {
  printf("%-6s | %8.0f", label, cpu_time);
  if (cpu_timer_freq) {
    f64 seconds = seconds_from_cpu_time(cpu_time, cpu_timer_freq);
    printf(" | %10f", 1000.0f*seconds);

    if (byte_count) {
      f64 gigabyte = (1024.0f * 1024.0f * 1024.0f);
      f64 best_bandwidth = byte_count / (gigabyte * seconds);
      printf(" | %10f", best_bandwidth);
    }
  }
}

static void print_time(char const *label, u64 cpu_time, u64 cpu_timer_freq, u64 byte_count) {
  print_time(label, (f64)cpu_time, cpu_timer_freq, byte_count);
}

static void print_results(RepTestResults results, u64 cpu_timer_freq, u64 byte_count) {
  print_time("Min", (f64)results.min_time, cpu_timer_freq, byte_count);
  printf("\n");

  print_time("Max", (f64)results.max_time, cpu_timer_freq, byte_count);
  printf("\n");

  if (results.test_count) {
    print_time("Avg", (f64)results.total_time / (f64)results.test_count, cpu_timer_freq, byte_count);
    printf("\n\n");
  }
}

static void error(RepTester *tester, char const *Message) {
  tester->test_mode = RepTestMode::Error;
  fprintf(stderr, "ERROR: %s\n", Message);
}

static void new_test_wave(RepTester *tester, u64 target_processed_byte_count, u64 cpu_timer_freq, u32 secondsToTry = 10) {
  if (tester->test_mode == RepTestMode::Uninitialized) {
    tester->test_mode = RepTestMode::Testing;
    tester->target_processed_byte_count = target_processed_byte_count;
    tester->cpu_timer_freq = cpu_timer_freq;
    tester->print_new_minimums = true;
    tester->results.min_time = (u64)-1;
  } else if (tester->test_mode == RepTestMode::Completed) {
    tester->test_mode = RepTestMode::Testing;

    if (tester->target_processed_byte_count != target_processed_byte_count) {
      error(tester, "target_processed_byte_count changed");
    }

    if (tester->cpu_timer_freq != cpu_timer_freq) {
      error(tester, "CPU frequency changed");
    }
  }

  tester->try_for_time = secondsToTry*cpu_timer_freq;
  tester->tests_started_at = read_cpu_timer();
}

static void begin_time(RepTester *tester) {
  ++tester->open_block_count;
  tester->time_accumulated_on_this_test -= read_cpu_timer();
}

static void end_time(RepTester *tester) {
  ++tester->close_block_count;
  tester->time_accumulated_on_this_test += read_cpu_timer();
}

static void count_bytes(RepTester *tester, u64 byte_count) {
  tester->bytes_accumulated_on_this_test += byte_count;
}

static bool is_testing(RepTester *tester) {
  if (tester->test_mode == RepTestMode::Testing) {
    u64 current_time = read_cpu_timer();

    if (tester->open_block_count) { // NOTE(casey): We don't count tests that had no timing blocks - we assume they took some other path
      if (tester->open_block_count != tester->close_block_count) {
        error(tester, "Unbalanced begin_time/end_time");
      }

      if (tester->bytes_accumulated_on_this_test != tester->target_processed_byte_count) {
        error(tester, "Processed byte count mismatch");
      }

      if (tester->test_mode == RepTestMode::Testing) {
        RepTestResults *results = &tester->results;
        u64 elapsed_time = tester->time_accumulated_on_this_test;
        results->test_count += 1;
        results->total_time += elapsed_time;

        if (results->max_time < elapsed_time) {
          results->max_time = elapsed_time;
        }

        if (results->min_time > elapsed_time) {
          results->min_time = elapsed_time;

          tester->tests_started_at = current_time;

          if (tester->print_new_minimums) {
            print_time("Min", results->min_time, tester->cpu_timer_freq, tester->bytes_accumulated_on_this_test);
            printf("               \r");
          }
        }

        tester->open_block_count = 0;
        tester->close_block_count = 0;
        tester->time_accumulated_on_this_test = 0;
        tester->bytes_accumulated_on_this_test = 0;
      }
    }

    if ((current_time - tester->tests_started_at) > tester->try_for_time) {
      tester->test_mode = RepTestMode::Completed;

      printf("%-6s | %8s | %10s | %10s \n", "Stat", "counts", "time (ms)", "speed (gb/s)");
      printf("----------------------------------------------\n");
      print_results(tester->results, tester->cpu_timer_freq, tester->target_processed_byte_count);
    }
  }

  bool result = (tester->test_mode == RepTestMode::Testing);
  return result;
}

#endif // _RepTester_HPP_

