#ifndef __FPOP_H__
#define __FPOP_H__

#if defined(__cplusplus)
extern "C" {
#endif  //__cplusplus

#include <assert.h>
#include <errno.h>
#include <numa.h>
#include <papi.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "padding.h"
#include "timing_stats.h"
#include "utils.h"

//#define DEBUG_PRINT

// 48 bit version | 8 bit tail | 8 bit lock mode

// 0: unlocked
// 1: locked
#define KOMB_LOCKED 1ULL           /* A writer holds the lock */
#define SET_OPT_LOCKED_READER 3ULL /* Locked + OPT Bit*/
#define UNSET_OPT_LOCKED_READER KOMB_LOCKED

#define kQueueNodeOffset 0
#define kQueueNodeIdBits 16
#define KOMB_STATUS_BITS 16

#define KOMB_COMPACT_LOCK_OFFSET (kQueueNodeIdBits)
#define QUEUE_NODES_COUNT ((1ULL << 10)) //kQueueNodeIdBits))

// all node id bits set := NULL
#define INVALID_NODE ((QUEUE_NODES_COUNT - 1))
#define LOCK_TAIL_IS_NULL(lock_ptr) (v->offset == INVALID_NODE)

#define INVALID_VER (INVALID_NODE)
#define kVersionStride (1ULL << (kQueueNodeOffset + kQueueNodeIdBits + KOMB_STATUS_BITS))
#define KOMB_LOCKED_MASK (KOMB_LOCKED)

#define _WAITER_UNPROCESSED 0U
#define _WAITER_PARKED 1U
#define _WAITER_PROCESSING 2U
#define _WAITER_PROCESSED 4U

#define MAX_SOCKET_ID 511

typedef struct {
  union {
    struct {
      volatile uint16_t offset;
      volatile uint16_t status;
      uint8_t version_count[4];
    };
    uint64_t to_u64;
  };
} komb_version_t;

typedef struct {
  union {
    uint64_t cnts;
    struct {
      union {
        struct {
          volatile uint16_t offset;
          volatile uint16_t status;
        };
        uint32_t offset_and_status;
      };
      uint8_t version[4];
    };
  };
} komb_op_t;

typedef struct komb_node {
  union {
    union {
      struct {
        volatile uint8_t completed;
        volatile uint8_t locked;
      };
      volatile uint16_t locked_completed;
    };
    uint64_t op_byte;
    char dummy1[64];
  };
  union {
    struct {
      struct komb_node *volatile next;
#if defined(ART_OFP_LOCK)
      void *art_k;
      void *art_parentNode;
      void *art_node;
      uint64_t art_new_tid;
      uint64_t art_v;
#else
      void *tree_node;
      void *tree_next;
      uint64_t tree_key;
      uint64_t tree_payload;
      uint64_t tree_versionNode;
#endif
      int result;
      int socket_id;
      int offset;
    };
    char dummy2[64];
  };
} komb_node_t __attribute__((aligned(CACHELINE_SIZE)));

//LOCK_EXTERN_TIMING_VAR(tclock_combiner_loop);
extern thread_local uint64_t tclock_combiner_loop_start; 
extern thread_local uint64_t write_cs_start;

extern thread_local unsigned int cur_thread_id;
extern thread_local unsigned int komb_cur_numa_id;

extern long komb_batch_size;

extern thread_local komb_node_t *volatile local_queue_head;
extern thread_local komb_node_t *volatile local_queue_tail;

extern komb_node_t *komb_base_qnode;
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;

#if defined(ART_OFP_LOCK)
extern thread_local int (*art_op)(void *, uint64_t, void *, void *, uint64_t);
#else
extern thread_local int (*btree_op)(void *, uint64_t, void *, uint64_t, uint64_t);
#endif

__always_inline bool komb_is_locked(komb_version_t v) { return (v.status & KOMB_LOCKED_MASK); }

__always_inline komb_version_t acq_tail(const komb_op_t *tail) {
  komb_op_t acq = {.cnts = __atomic_load_n(&tail->cnts, __ATOMIC_ACQUIRE)};
  komb_version_t ver = {.to_u64 = acq.cnts};
  return ver;
}

__always_inline komb_version_t komb_try_begin_read(const komb_op_t *tail, bool *restart) {
  komb_version_t last = acq_tail(tail);
#ifdef OP_READ
  *restart = komb_is_locked(last) && (last.status == UNSET_OPT_LOCKED_READER);
#else
  *restart = komb_is_locked(last);
#endif
  return last;
}

static __always_inline komb_node_t *get_node_at_offset(uint16_t offset) {
  return &komb_base_qnode[offset];
}

__always_inline void komb_turn_off_opt_reads(komb_op_t *tail) {
#ifdef OP_READ
  tail->status = UNSET_OPT_LOCKED_READER;
#endif
}

__always_inline void komb_turn_on_opt_reads(komb_op_t *tail) {
#ifdef OP_READ
  tail->status = SET_OPT_LOCKED_READER;
#endif
}

static __always_inline void check_and_set_combiner(komb_op_t *tail) {
  while (true) {
    while (tail->status == KOMB_LOCKED)
      ;
#ifdef OP_READ
    uint16_t new_val = SET_OPT_LOCKED_READER;
#else
    uint16_t new_val = KOMB_LOCKED;
#endif
    if (smp_cas(&tail->status, 0, new_val) == 0) {
      return;
    }
  }
}

__always_inline static void komb_spin_lock_mcs(komb_op_t *tail) {
  komb_node_t *prev_node = NULL, *next_node = NULL;
  komb_node_t *curr_node = my_local_komb_node;

  curr_node->locked = true;
  curr_node->completed = _WAITER_UNPROCESSED;
  curr_node->next = NULL;
  curr_node->socket_id = MAX_SOCKET_ID;

  uint16_t prev = smp_swap(&tail->offset, komb_node_offset);
  if (prev != INVALID_NODE) {
    prev_node = get_node_at_offset(prev);
    prev_node->next = curr_node;
    while (curr_node->locked) {
      CPU_PAUSE();
    }
    BUG_ON(curr_node->completed == _WAITER_PROCESSED);
  }

#ifdef OP_READ
  komb_op_t expected = {.offset = (uint16_t)komb_node_offset, .status = SET_OPT_LOCKED_READER};
#else
  komb_op_t expected = {.offset = (uint16_t)komb_node_offset, .status = KOMB_LOCKED};
#endif
  komb_op_t locked = {.offset = INVALID_NODE, .status = KOMB_LOCKED};
  check_and_set_combiner(tail);

  if (curr_node->next == NULL) {
    if ((tail->offset == komb_node_offset) &&
        smp_cas(&tail->offset_and_status, expected.offset_and_status, locked.offset_and_status) ==
            expected.offset_and_status) {
      goto release;
    }
  }

  while ((next_node = curr_node->next) == NULL)
    ;
  BUG_ON(next_node == NULL);

  next_node->locked = false;
release:
  komb_turn_off_opt_reads(tail);
}

__always_inline void komb_mutex_lock(komb_op_t *tail) {
  while(true) {
    while(tail->status != 0)
      CPU_PAUSE();

    if (smp_cas(&tail->status, 0, KOMB_LOCKED) == 0)
    {
      //printf("Lock: %p %d acquired in lock function\n", tail, komb_node_offset);
      return;
    }
  }

  //komb_spin_lock_mcs(tail);
}

__always_inline bool komb_mutex_trylock(komb_op_t *tail) {
  bool res = (smp_cas(&tail->status, 0, KOMB_LOCKED) == 0);
  //printf("Lock: %p %d trylock result: %d\n", tail, cur_thread_id,  res);
  //if(tclock_combiner_loop_start++ == 100)
  //  exit(-1);
  return res;
  //return (smp_cas(&tail->status, 0, SET_OPT_LOCKED_READER) == 0);
}

__always_inline bool komb_mutex_trylock_version(komb_op_t *tail, uint64_t version) {
  komb_version_t expected = {.to_u64 = version};
  komb_version_t locked = {.to_u64 = version};
  locked.status = KOMB_LOCKED;
  bool res = (smp_cas(&tail->cnts, version, locked.to_u64) == version);
  return res;
}

__always_inline void __komb_mutex_unlock(komb_op_t *tail) {
  __atomic_fetch_add(&tail->cnts, (kVersionStride - (KOMB_LOCKED << KOMB_COMPACT_LOCK_OFFSET)),
                     __ATOMIC_SEQ_CST);
  //printf("Lock: %p %d release in unlock function\n", tail, komb_node_offset);
  return;
}

__always_inline void komb_mutex_unlock(komb_op_t *tail) { __komb_mutex_unlock(tail); }

__always_inline bool komb_validate_read(const komb_op_t *tail, const komb_version_t version) {
  komb_version_t curr_version = acq_tail(tail);
  bool read_ok = curr_version.to_u64 == version.to_u64;
  return read_ok;
}

__always_inline int execute_op(komb_node_t *my_node) {
#if defined(ART_OFP_LOCK)
  return (*art_op)(my_node->art_k, my_node->art_new_tid, my_node->art_node, my_node->art_parentNode,
                   my_node->art_v);
#else
  return (*btree_op)(my_node->tree_node, my_node->tree_versionNode, my_node->tree_next,
                     my_node->tree_key, my_node->tree_payload);
#endif
}

__always_inline static void add_to_local_queue(komb_node_t *node) {
  if (local_queue_head == NULL) {
    local_queue_head = node;
    local_queue_tail = node;
  } else {
    local_queue_tail->next = node;
    local_queue_tail = node;
  }
  //printf("combiner: %d moving to local_queue head: %d tail: %d node: %d\n",
  //    cur_thread_id,  
   //   local_queue_head->offset, local_queue_tail->offset, node->offset);
}

__always_inline static bool check_combiner_condition(komb_node_t *node) {
  return (node == NULL || node->socket_id == MAX_SOCKET_ID || node->next == NULL);
}

__always_inline static komb_node_t *get_next_node(komb_node_t *my_node) {
  komb_node_t *next_node = my_node->next;

  while (true) {
    if (check_combiner_condition(next_node)) goto next_node_null;

    if (next_node->socket_id == komb_cur_numa_id) {
      PREFETCH((next_node->next));
#if defined(ART_OFP_LOCK)
      PREFETCH((next_node->art_k));
      PREFETCH((next_node->art_parentNode));
      PREFETCH((next_node->art_node));
#endif
      return next_node;
    }

    add_to_local_queue(next_node);
    next_node = next_node->next;
  }

next_node_null:
  return next_node;
}

static inline int current_numa_node() {
  return numa_node_of_cpu(sched_getcpu());  // FIXME: use rdtscp
}

__always_inline int komb_mutex_slowpath(komb_op_t *tail) {
  komb_node_t *curr_node, *prev_node, *next_node;
  uint64_t counter_val;
  int res;

  curr_node = my_local_komb_node;

  curr_node->locked = true;
  curr_node->completed = false;
  curr_node->next = NULL;
  curr_node->socket_id = current_numa_node();
  curr_node->offset = komb_node_offset;
  curr_node->result = 0;

  uint16_t prev = smp_swap(&tail->offset, komb_node_offset);
  if (prev != INVALID_NODE) {
    prev_node = get_node_at_offset(prev);
#if defined(DEBUG_PRINT)
    printf("time: %ld Lock: %p %d Waiter %d spinning prev: %d socket_id: %d\n", locktime_timing_start(), 
        tail, cur_thread_id,  curr_node->offset, prev_node->offset, komb_cur_numa_id);
#endif
    prev_node->next = curr_node;
    smp_mb();
    while (curr_node->locked)
      ;

    if (curr_node->completed) return curr_node->result;
  }

#ifdef OP_READ
  uint16_t new_val = SET_OPT_LOCKED_READER;
#else
  uint16_t new_val = KOMB_LOCKED;
#endif

 #if defined(DEBUG_PRINT)
 printf("time: %ld Lock: %p %d Combiner: %d acquiring the lock\n", locktime_timing_start(),  
      tail, cur_thread_id,  curr_node->offset);
#endif

  while (true) {
    while (tail->status != 0)
      ;

    PREFETCH((curr_node->next));

    if (smp_cas(&tail->status, 0, new_val) == 0) break;
  }

 #if defined(DEBUG_PRINT)
 printf("time: %ld Lock: %p %d Combiner: %d got the lock\n", locktime_timing_start(),
      tail, cur_thread_id,  curr_node->offset);
#endif

#ifdef OP_READ
  komb_op_t expected = {.offset = (uint16_t)komb_node_offset, .status = SET_OPT_LOCKED_READER};
#else
  komb_op_t expected = {.offset = (uint16_t)komb_node_offset, .status = KOMB_LOCKED};
#endif
  komb_op_t locked = {.offset = INVALID_NODE, .status = KOMB_LOCKED};

  if (curr_node->next == NULL) {
    if ((tail->offset == komb_node_offset) &&
        smp_cas(&tail->offset_and_status, expected.offset_and_status, locked.offset_and_status) ==
            expected.offset_and_status) {
 #if defined(DEBUG_PRINT)
     printf("time: %ld Lock: %p %d Combiner: %d only one in queue\n", locktime_timing_start(), 
          tail, cur_thread_id,  curr_node->offset);
 #endif
     goto execute_cs;
    }
  }

  while ((next_node = curr_node->next) == NULL)
    ;

 #if defined(DEBUG_PRINT)
 printf("time: %ld Lock: %p %d Combiner: %d next_node: %d\n", locktime_timing_start(),
      tail, cur_thread_id,  curr_node->offset, next_node->offset);
#endif

  curr_node = next_node; // Execute myself at last.

  if (check_combiner_condition(curr_node)) {
 #if defined(DEBUG_PRINT)
   printf("time: %ld Lock: %p Combiner: %d passing ownership to: %d\n", locktime_timing_start(), 
        tail, cur_thread_id,  curr_node->offset);
 #endif
   curr_node->locked = false;
    smp_mb();
    goto execute_cs;
  }

  counter_val = 0;
  local_queue_head = NULL;
  local_queue_tail = NULL;

  komb_cur_numa_id = current_numa_node(); 

  while (true) {
    counter_val++;
    PREFETCHW((curr_node + 64));
    next_node = get_next_node(curr_node);
    PREFETCHW((curr_node));
    komb_turn_off_opt_reads(tail);
    LOCK_START_TIMING(write_cs);
    curr_node->result = execute_op(curr_node);
    LOCK_END_TIMING(write_cs);
    komb_turn_on_opt_reads(tail);
    curr_node->locked_completed = 1;
 #if defined(DEBUG_PRINT)
   printf("time: %ld Lock: %p Combiner: %d waking up: %d\n", locktime_timing_start(), 
        tail, cur_thread_id,  curr_node->offset);
#endif

    if (check_combiner_condition(next_node) || counter_val > komb_batch_size) break;

    LOCK_END_TIMING(tclock_combiner_loop);
    LOCK_START_TIMING(tclock_combiner_loop);

    curr_node = next_node;
  }

  ADD_TO_STATS(tclock_waiter_combined, counter_val);

  if (local_queue_head != NULL) {
  #if defined(DEBUG_PRINT)
  printf("time: %ld Lock: %p Combiner: %d local_queue_head: %d local_queue_tail: %d\n", locktime_timing_start(), 
        tail, cur_thread_id,  local_queue_head->offset, local_queue_tail->offset);
 #endif
   local_queue_tail->next = next_node;
    next_node = local_queue_head;
    local_queue_head = NULL;
    local_queue_tail = NULL;
  }

  next_node->locked = false;
 #if defined(DEBUG_PRINT)
 printf("time: %ld Lock: %p Combiner: %d after combining passing ownership to: %d\n", locktime_timing_start(), 
      tail, cur_thread_id,  next_node->offset);
#endif

execute_cs:
  komb_turn_off_opt_reads(tail);
  res = execute_op(my_local_komb_node);
  __atomic_fetch_add(&tail->cnts, (kVersionStride - (KOMB_LOCKED << KOMB_COMPACT_LOCK_OFFSET)),
                     __ATOMIC_SEQ_CST);
 #if defined(DEBUG_PRINT)
 printf("time: %ld Lock: %p %d releasing the lock\n", locktime_timing_start(), 
      tail, komb_node_offset);
 #endif
 return res;
}

__always_inline int komb_mutex_execute(komb_op_t *tail) {
  if (tail->status != 0 || smp_cas(&tail->status, 0, KOMB_LOCKED) != 0) {
 #if defined(DEBUG_PRINT)
   printf("time: %ld Lock: %p %d going to slowpath\n", locktime_timing_start(), 
      tail, komb_node_offset);
  #endif
  return komb_mutex_slowpath(tail);
  }

 #if defined(DEBUG_PRINT)
 printf("time: %ld Lock: %p %d acquired in fast path\n", locktime_timing_start(), 
      tail, komb_node_offset);
 #endif
 int res = execute_op(my_local_komb_node);

  __atomic_fetch_add(&tail->cnts, (kVersionStride - (KOMB_LOCKED << KOMB_COMPACT_LOCK_OFFSET)),
                     __ATOMIC_SEQ_CST);
 #if defined(DEBUG_PRINT)
 printf("time: %ld Lock: %p %d releasing the lock fast path\n", locktime_timing_start(),
      tail, komb_node_offset);
 #endif
 return res;
}

static void komb_thread_start() {
  komb_cur_numa_id = current_numa_node();
  //printf("fpop_start threadid: %d qnodeid: %d numaid: %d cpuid: %d\n", 
  //		  cur_thread_id, komb_node_offset, komb_cur_numa_id, sched_getcpu()); 
  BUG_ON(komb_base_qnode == NULL);
  local_queue_head = NULL;
  local_queue_tail = NULL;

#if defined(ART_OFP_LOCK)
  art_op = NULL;
#else
  btree_op = NULL;
#endif
}

static void komb_thread_exit(void) {}

static void komb_application_init(void) {}

static void komb_application_exit(void) {}

#if defined(__cplusplus)
}
#endif  // __cplusplus
#endif  // __FPOP_H__
