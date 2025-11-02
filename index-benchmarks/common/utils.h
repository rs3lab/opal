/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2016 Hugo Guiroux <hugo.guiroux at gmail dot com>
 *               2013 Tudor David
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of his software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include <malloc.h>
#include <padding.h>
#include <stdint.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

#ifndef __UTILS_H__
#define __UTILS_H__

//#include <topology.h>

#define MAX_THREADS 2048
#define CPU_PAUSE() asm volatile("pause\n" : : : "memory")
#define COMPILER_BARRIER() asm volatile("" : : : "memory")
#define MEMORY_BARRIER() __sync_synchronize()
#define REP_VAL 23

#define PREFETCHW(x) asm volatile("prefetchw %0" ::"m"(*(unsigned long *)x))
#define PREFETCH(x) asm volatile("prefetch %0" ::"m"(*(unsigned long *)x))

#ifdef UNUSED
#elif defined(__GNUC__)
#define UNUSED(x) UNUSED_##x __attribute__((unused))
#elif defined(__LCLINT__)
#define UNUSED(x) /*@unused@*/ x
#else
#define UNUSED(x) x
#endif

//#define DEBUG(...)                        fprintf(stderr, ## __VA_ARGS__)
#define DEBUG(...)
//#define DEBUG_PTHREAD(...)                        fprintf(stderr, ##
//__VA_ARGS__)
#define DEBUG_PTHREAD(...)

//#define WAITER_DEBUG

/* debugging */
#ifdef WAITER_DEBUG
typedef enum {
  RED,
  GREEN,
  BLUE,
  MAGENTA,
  YELLOW,
  CYAN,
  END,
} color_num;

static char colors[END][8] = {
    "\x1B[31m", "\x1B[32m", "\x1B[34m", "\x1B[35m", "\x1b[33m", "\x1b[36m",
};
static unsigned long counter = 0;

#define dprintf(__fmt, ...)                                                                        \
  do {                                                                                             \
    smp_faa(&counter, 1);                                                                          \
    fprintf(stderr, "%s [DBG:%010lu: %d %p(%016llx) (%s: %d)]: " __fmt, colors[node_offset % END], \
            counter, node_offset, tail, acq_tail(tail), __func__, __LINE__, ##__VA_ARGS__);        \
    fflush(stderr);                                                                                \
  } while (0);

#include <stdlib.h>

#define BUG_ON(v)                             \
  if ((v)) {                                  \
    printf("(%s: %d)\n", __func__, __LINE__); \
    exit(1);                                  \
  }

#else
#define dprintf(__fmt, ...) \
  do {                      \
  } while (0)
#define BUG_ON(v) \
  do {            \
  } while (0)
#endif

#define __scalar_type_to_expr_cases(type) \
  unsigned type : (unsigned type)0, signed type : (signed type)0

#define __unqual_scalar_typeof(x)                                                              \
  typeof(_Generic((x), char                                                                    \
                  : (char)0, __scalar_type_to_expr_cases(char),                                \
                    __scalar_type_to_expr_cases(short), __scalar_type_to_expr_cases(int),      \
                    __scalar_type_to_expr_cases(long), __scalar_type_to_expr_cases(long long), \
                    default                                                                    \
                  : (x)))

#define smp_cond_load_relaxed(ptr, cond_expr) \
  ({                                          \
    typeof(ptr) __PTR = (ptr);                \
    __unqual_scalar_typeof(*ptr) VAL;         \
    for (;;) {                                \
      VAL = READ_ONCE(*__PTR);                \
      if (cond_expr) break;                   \
      CPU_PAUSE();                            \
    }                                         \
    (typeof(*ptr))VAL;                        \
  })

static inline void smp_rmb(void) { __asm __volatile("lfence" ::: "memory"); }

static inline void smp_cmb(void) { __asm __volatile("" ::: "memory"); }

#define barrier() smp_cmb()

static inline void smp_wmb(void) { __asm __volatile("sfence" ::: "memory"); }

static inline void smp_mb(void) { __asm __volatile("mfence" ::: "memory"); }

#define false 0
#define true 1

#define atomic_andnot(val, ptr) __sync_fetch_and_and((ptr), ~(val));
#define atomic_fetch_or_acquire(val, ptr) __sync_fetch_and_or((ptr), (val));

static inline void __write_once_size(volatile void *p, void *res, int size) {
  switch (size) {
    case 1:
      *(volatile uint8_t *)p = *(uint8_t *)res;
      break;
    case 2:
      *(volatile uint16_t *)p = *(uint16_t *)res;
      break;
    case 4:
      *(volatile uint32_t *)p = *(uint32_t *)res;
      break;
    case 8:
      *(volatile uint64_t *)p = *(uint64_t *)res;
      break;
    default:
      barrier();
      memcpy((void *)p, (const void *)res, size);
      barrier();
  }
}

static inline void __read_once_size(volatile void *p, void *res, int size) {
  switch (size) {
    case 1:
      *(uint8_t *)res = *(volatile uint8_t *)p;
      break;
    case 2:
      *(uint16_t *)res = *(volatile uint16_t *)p;
      break;
    case 4:
      *(uint32_t *)res = *(volatile uint32_t *)p;
      break;
    case 8:
      *(uint64_t *)res = *(volatile uint64_t *)p;
      break;
    default:
      barrier();
      memcpy((void *)res, (const void *)p, size);
      barrier();
  }
}

#define WRITE_ONCE(x, val)                       \
  ({                                             \
    union {                                      \
      typeof(x) __val;                           \
      char __c[1];                               \
    } __u = {.__val = (typeof(x))(val)};         \
    __write_once_size(&(x), __u.__c, sizeof(x)); \
    __u.__val;                                   \
  })

#define READ_ONCE(x)                            \
  ({                                            \
    union {                                     \
      typeof(x) __val;                          \
      char __c[1];                              \
    } __u;                                      \
    __read_once_size(&(x), __u.__c, sizeof(x)); \
    __u.__val;                                  \
  })

#define smp_cas(__ptr, __old_val, __new_val) \
  __sync_val_compare_and_swap(__ptr, __old_val, __new_val)
#define smp_swap(__ptr, __val) __sync_lock_test_and_set(__ptr, __val)
#define smp_faa(__ptr, __val) __sync_fetch_and_add(__ptr, __val)
#define smp_aaf(__ptr, __val) __sync_add_and_fetch(__ptr, __val)

struct t_info {
  int tid;
  unsigned int banned;
  unsigned long banned_until;
  unsigned long start_ticks;
  unsigned long cs_start_ticks;
  unsigned long vcs_runtime;
};

void *alloc_cache_align(size_t n);

static inline void *xchg_64(void *ptr, void *x) {
  __asm__ __volatile__("xchgq %0,%1"
                       : "=r"((unsigned long long)x)
                       : "m"(*(volatile long long *)ptr), "0"((unsigned long long)x)
                       : "memory");

  return x;
}

static inline unsigned xchg_32(void *ptr, unsigned x) {
  __asm__ __volatile__("xchgl %0,%1"
                       : "=r"((unsigned)x)
                       : "m"(*(volatile unsigned *)ptr), "0"(x)
                       : "memory");

  return x;
}

// test-and-set uint8_t, from libslock
static inline uint8_t l_tas_uint8(volatile uint8_t *addr) {
  uint8_t oldval;
  __asm__ __volatile__("xchgb %0,%1"
                       : "=q"(oldval), "=m"(*addr)
                       : "0"((unsigned char)0xff), "m"(*addr)
                       : "memory");
  return (uint8_t)oldval;
}

static inline uint64_t rdpmc(unsigned int counter) {
  uint32_t low, high;

  asm volatile("rdpmc" : "=a"(low), "=d"(high) : "c"(counter));

  return low | ((uint64_t)high) << 32;
}

static inline uint64_t rdtsc(void) {
  uint32_t low, high;

  asm volatile("rdtsc" : "=a"(low), "=d"(high));

  return low | ((uint64_t)high) << 32;
}

// EPFL libslock
#define my_random xorshf96
#define getticks rdtsc
typedef uint64_t ticks;

static inline unsigned long xorshf96(unsigned long *x, unsigned long *y,
                                     unsigned long *z) {  // period 2^96-1
  unsigned long t;
  (*x) ^= (*x) << 16;
  (*x) ^= (*x) >> 5;
  (*x) ^= (*x) << 1;

  t = *x;
  (*x) = *y;
  (*y) = *z;
  (*z) = t ^ (*x) ^ (*y);

  return *z;
}

static inline void cdelay(ticks cycles) {
  ticks __ts_end = getticks() + (ticks)cycles;
  while (getticks() < __ts_end)
    ;
}

static inline unsigned long *seed_rand() {
  unsigned long *seeds;
  int num_seeds = CACHELINE_SIZE / sizeof(unsigned long);
  if (num_seeds < 3) num_seeds = 3;

  seeds = (unsigned long *)memalign(CACHELINE_SIZE, num_seeds * sizeof(unsigned long));
  seeds[0] = getticks() % 123456789;
  seeds[1] = getticks() % 362436069;
  seeds[2] = getticks() % 521288629;
  return seeds;
}

static inline void nop_rep(uint32_t num_reps) {
  uint32_t i;
  for (i = 0; i < num_reps; i++) {
    asm volatile("NOP");
  }
}

static inline void pause_rep(uint32_t num_reps) {
  uint32_t i;
  for (i = 0; i < num_reps; i++) {
    CPU_PAUSE();
    /* PAUSE; */
    /* asm volatile ("NOP"); */
  }
}

static inline void wait_cycles(uint64_t cycles) {
  if (cycles < 256) {
    cycles /= 6;
    while (cycles--) {
      CPU_PAUSE();
    }
  } else {
    ticks _start_ticks = getticks();
    ticks _end_ticks = _start_ticks + cycles - 130;
    while (getticks() < _end_ticks)
      ;
  }
}
#endif
