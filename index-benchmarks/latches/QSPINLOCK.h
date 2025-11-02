#pragma once

#include <glog/logging.h>
#include <immintrin.h>
#include <stdint.h>

#include <atomic>
#include <cassert>

#include "qspinlock.h"

thread_local qspinlock_node_t *my_local_qspinlock_node;
thread_local unsigned int qspinlock_node_offset;
qspinlock_node_t *qspinlock_base_qnode;

namespace qspin_lock {

inline qspinlock_node_t *base_qnode = nullptr;

class QSpinLock {
 public:
  static constexpr uint64_t kNumQueueNodes = 255;
  static constexpr uint64_t invalid_version = -1;
  QSpinLock() {
    lock_.cnts = 0;
    lock_.offset = INVALID_NODE;
  }

  inline void lock(void* = nullptr) {
    qspinlock_mutex_lock(&lock_);
  }

  inline void unlock(void* = nullptr) {
    qspinlock_mutex_unlock(&lock_);
  }

  inline void read_lock(void* = nullptr) {
    qspinlock_mutex_lock(&lock_);
  }

  inline void read_unlock(void* = nullptr) {
    qspinlock_mutex_unlock(&lock_);
  }

  private:
    qspinlock_mutex_t lock_;
};

}  // namespace mcs
