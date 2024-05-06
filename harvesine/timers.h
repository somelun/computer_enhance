#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <x86intrin.h>
#include <mach/mach_time.h>

static inline unsigned long get_cpu_freq() {
  uint64_t freq = 0;
  size_t size = sizeof(freq);

  if (sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0) < 0) {
    // perror("sysctl");
  }

  return freq;
}

static inline unsigned long get_os_timer_freq() {
  return 0;
}

static inline unsigned long read_os_timer() {
  return mach_absolute_time();
}

static inline unsigned long read_cpu_timer() {
  return __rdtsc();
}

// just an experiment, but it gives the same result as __rdtcs()
static inline unsigned long asm_rdtsc() {
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));

  return ((unsigned long long)lo) | ( ((unsigned long long)hi) << 32);
}

#endif // TIMERS_H

