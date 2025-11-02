#ifndef __TIMING_STATS_H_
#define __TIMING_STATS_H_

#if defined(__cplusplus)
extern "C" {
#define THREAD_LOCAL thread_local
#else
#define THREAD_LOCAL _Thread_local
#endif  //__cplusplus

#include <asm/msr.h>
#include <stdbool.h>

#include "stats.h"
#include "utils.h"

#define TIME_UPPER_BOUND 16384  // 1048576

static inline uint64_t locktime_timing_start(void) {
  uint64_t rax, rdx;
  __asm__ __volatile__("rdtscp\n" : "=a"(rax), "=d"(rdx) : : "%ecx");
  return (rdx << 32) + rax;
}

static inline uint64_t locktime_timing_end(void) {
  uint64_t rax, rdx;
  __asm__ __volatile__("rdtscp\n" : "=a"(rax), "=d"(rdx) : : "%ecx");
  return (rdx << 32) + rax;
}

#ifdef LOCK_MEASURE_TIME

#define LOCK_DEFINE_TIMING_VAR(name) inline thread_local uint64_t name##_start
#define LOCK_EXTERN_TIMING_VAR(name) extern thread_local uint64_t name##_start

#define LOCK_START_TIMING(name)             \
  do {                                      \
    name##_start = locktime_timing_start(); \
    barrier();                              \
  } while (0)

#define LOCK_END_TIMING(name)                                \
  do {                                                       \
    uint64_t name##_diff;                                    \
    name##_diff = (locktime_timing_end() - name##_start);    \
    barrier();                                               \
    if (name##_diff > 0 && name##_diff < TIME_UPPER_BOUND) { \
      ADD_TO_STATS(name, name##_diff);                       \
    }                                                        \
  } while (0)

#else  // LOCK_MEASURE_TIME

#define LOCK_DEFINE_TIMING_VAR(name)
#define LOCK_EXTERN_TIMING_VAR(name)
#define LOCK_START_TIMING(name)
#define LOCK_END_TIMING(name)

#endif  // LOCK_MEASURE_TIME

#if defined(__cplusplus)
}
#endif  //__cplusplus

#endif /* __TIMING_STATS_H_ */
