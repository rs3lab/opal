#ifndef __STATS_H__
#define __STATS_H__

#include <pthread.h>
#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

#define NUM_BUCKETS 5  // 0-10, 10-100, 100-1000, 1000-10000, >10000

typedef struct stats {
#define X(field, str) uint64_t field;
#define Y(field, str)     \
  uint64_t field;         \
  uint64_t field##_count; \
  uint64_t field##_dist[NUM_BUCKETS];
#include "all-stats-fields.h"
#undef X
#undef Y
} stats_t __attribute__((aligned(64)));

extern __thread stats_t my_stats;
extern stats_t aggregate_stats;

void print_thread_stats(int thread_id);
void reset_thread_stats(int thread_id);
void aggregate_my_stats(int thread_id);

void print_aggregate_stats(void);
void reset_aggregate_stats(void);

#define ADD_TO_STATS_WITHOUT_DIST(stats_name, stats_value) \
  ({                                          \
    my_stats.stats_name += stats_value;       \
  })


#define ADD_TO_STATS(stats_name, stats_value) \
  ({                                          \
    my_stats.stats_name += stats_value;       \
    my_stats.stats_name##_count++;            \
    if (stats_value <= 10)                    \
      my_stats.stats_name##_dist[0]++;        \
    else if (stats_value <= 100)              \
      my_stats.stats_name##_dist[1]++;        \
    else if (stats_value <= 1000)             \
      my_stats.stats_name##_dist[2]++;        \
    else if (stats_value <= 10000)            \
      my_stats.stats_name##_dist[3]++;        \
    else                                      \
      my_stats.stats_name##_dist[4]++;        \
  })

#if defined(__cplusplus)
};
#endif

#endif  //__STATS_H__
