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
  return 0;
}

static inline unsigned long read_cpu_timer() {
  // uint64_t start = mach_absolute_time();
  // uint64_t stop = mach_absolute_time();
  //

  return __rdtsc();
}

// unsigned __int64 inline GetRDTSC() {
//    __asm {
//       ; Flush the pipeline
//       XOR eax, eax
//       CPUID
//       ; Get RDTSC counter in edx:eax
//       RDTSC
//    }
// }

#endif // TIMERS_H

