#pragma once

#include <unistd.h>

#include <atomic>
#include <cassert>
#include <iostream>
#include <memory>
#include <mutex>
#include <random>

#include "fpop.h"
#include "numa.h"

extern thread_local unsigned int node_offset;

inline thread_local unsigned int komb_cur_numa_id;
inline long komb_batch_size = 16384;
inline komb_node_t *komb_base_qnode;
inline thread_local komb_node_t *volatile local_queue_head;
inline thread_local komb_node_t *volatile local_queue_tail;
inline thread_local unsigned int komb_node_offset;
inline thread_local komb_node_t *my_local_komb_node;
inline thread_local bool is_local_queue_tail_last;

#if defined(ART_OFP_LOCK)
inline thread_local int (*art_op)(void *, uint64_t, void *, void *, uint64_t);
#else
inline thread_local int (*btree_op)(void *, uint64_t, void *, uint64_t, uint64_t);
#endif

inline thread_local uint64_t tclock_combiner_loop_start;

namespace tc_lock {

inline komb_node_t *base_qnode = nullptr;

class OpFP {
 public:
  static constexpr uint64_t invalid_version = INVALID_VER;
  static constexpr uint64_t kNumQueueNodes = QUEUE_NODES_COUNT;

  OpFP() {
    lock_.cnts = 0;
    lock_.offset = INVALID_NODE;
  }

  ~OpFP() {}

  inline bool is_locked() const { return lock_.status & KOMB_LOCKED_MASK; };

  inline uint64_t begin_read() const {
    while (true) {
      bool restart = false;
      uint64_t version = try_begin_read(restart);
      if (!restart) {
        return version;
      }
    }
  }

  inline uint64_t try_begin_read(bool &need_restart) const {
    komb_version_t ver = komb_try_begin_read(&lock_, &need_restart);
    return ver.to_u64;
  }

  inline bool validate_read(uint64_t version) const {
    bool res = komb_validate_read(&lock_, {.to_u64 = version});
    return res;
  }

  inline void write_lock() { komb_mutex_lock(&lock_); }

  inline bool try_write_lock() {
    bool res = komb_mutex_trylock(&lock_);
    return res;
  }

  inline bool try_write_lock(uint64_t &version) {
    bool res = komb_mutex_trylock_version(&lock_, version);
    return res;
  }

  inline void write_unlock() { komb_mutex_unlock(&lock_); }

  inline void read_unlock_or_restart(uint64_t start_version, bool &need_restart) const {
    need_restart = !validate_read(start_version);
  }

  inline void turn_off_opt_reads() { komb_turn_off_opt_reads(&lock_); }

#if defined(ART_OFP_LOCK)
  inline int execute(void *k, uint64_t new_tid, void *node, void *parentNode, uint64_t v,
                     int (*fnp)(void *, uint64_t, void *, void *, uint64_t)) {
    my_local_komb_node->art_k = k;
    my_local_komb_node->art_new_tid = new_tid;
    my_local_komb_node->art_node = node;
    my_local_komb_node->art_parentNode = parentNode;
    my_local_komb_node->art_v = v;

    art_op = fnp;

    return komb_mutex_execute(&lock_);
  }
#else
  inline int execute(void *node, uint64_t versionNode, void *next, uint64_t k, uint64_t v,
                     int (*fnp)(void *, uint64_t, void *, uint64_t, uint64_t)) {
    my_local_komb_node->tree_key = k;
    my_local_komb_node->tree_payload = v;
    my_local_komb_node->tree_versionNode = versionNode;
    my_local_komb_node->tree_node = node;
    my_local_komb_node->tree_next = next;

    btree_op = fnp;

    return komb_mutex_execute(&lock_);
  }
#endif

 private:
  komb_op_t lock_ = {0};
};
}  // namespace tc_lock
