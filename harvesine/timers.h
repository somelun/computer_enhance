#ifndef _TIMERS_H_
#define _TIMERS_H_

#include <stdint.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach_time.h>

#if defined(__x86_64__)
#include <x86intrin.h>
#endif

#if defined(__x86_64__)

static inline uint64_t read_os_timer_freq() {
  LARGE_INTEGER freq;
  QueryPerformanceFrequency(&freq);
  return freq.QuadPart;
}

static inline uint64_t read_os_timer() {
  LARGE_INTEGER value;
  QueryPerformanceCounter(&value);
  return value.QuadPart;
}

static inline uint64_t read_cpu_freq() {
  uint64_t freq = 0;
  size_t size = sizeof(freq);

  if (sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0) < 0) {
    // perror("sysctl");
  }

  return freq;
}

static inline uint64_t read_cpu_timer() {
  return __rdtsc();
}

// just an experiment, but it gives the same result as __rdtcs()
static inline uint64_t asm_rdtsc() {
  uint32_t hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  return ((uint64_t)lo) | ( ((uint64_t)hi) << 32);
}

#elif defined(__aarch64__)

static inline uint64_t read_os_timer_freq() {
  // this is nanoseconds https://developer.apple.com/documentation/kernel/1462446-mach_absolute_time
  return 1e9;
}

static inline uint64_t read_os_timer() {
  return mach_absolute_time();
}

static inline uint64_t read_cpu_freq() {
  uint64_t freq;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (freq));
  return freq;
}

static inline uint64_t read_cpu_timer() {
  uint64_t value;
  __asm__ volatile("isb;\n\tmrs %0, cntvct_el0" : "=r" (value) :: "memory");
  return value;
}

#endif

#endif // _TIMERS_H_

