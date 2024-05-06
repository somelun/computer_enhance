#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <mach/mach_time.h>

#if defined(__x86_64__)
#include <x86intrin.h>
#endif

static inline unsigned long get_cpu_freq() {
#if defined(__x86_64__)
  uint64_t freq = 0;
  size_t size = sizeof(freq);

  if (sysctlbyname("hw.cpufrequency", &freq, &size, NULL, 0) < 0) {
    // perror("sysctl");
  }

  return freq;

#elif defined(__aarch64__)
  uint64_t val;
  __asm__ volatile("mrs %0, cntfrq_el0" : "=r" (val));
  return val;
#endif
}

static inline unsigned long get_os_timer_freq() {
  return 0;
}

static inline unsigned long read_os_timer() {
  return mach_absolute_time();
}

static inline unsigned long read_cpu_timer() {
#if defined(__x86_64__)
  return __rdtsc();
#elif defined(__aarch64__)
  uint64_t val;
  // use isb to avoid speculative read of cntvct_el0
  __asm__ volatile("isb;\n\tmrs %0, cntvct_el0" : "=r" (val) :: "memory");
  return val;
#endif
}

#if defined(__x86_64__)
// just an experiment, but it gives the same result as __rdtcs()
static inline unsigned long asm_rdtsc() {
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));

  return ((unsigned long long)lo) | ( ((unsigned long long)hi) << 32);
}
#endif

#endif // TIMERS_H

