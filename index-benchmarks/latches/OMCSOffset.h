#pragma once

#include <numa.h>

#include <iostream>

#if defined(OMCS_LOCK)
#include "OMCSImpl.h"
#elif defined(MCSRW_LOCK) || defined(OPT_MCSRW_HYBRID_LOCK)
#include "MCSRW.h"
#elif defined(TC_LOCK_OP)
#include "OpTCL.h"
#elif defined(OFP_LOCK)
#include "OpFP.h"
#elif defined(OFP_MUTEX)
#include "OpFPMutex.h"
#elif defined(TC_LOCK_NDL_OP)
#include "OpTCL_NDL.h"
#elif defined(TC_LOCK)
#include "TCL.h"
#elif defined(TC_LOCK_RW)
#include "TCLRW.h"
#elif defined(TC_UNLOCK_OP)
#include "OpTCLUnlock.h"
#elif defined(TD_LOCK_OP)
#include "OpTDL.h"
#elif defined(AQS_LOCK_OP)
#include "OpAQS.h"
#elif defined(AQS_LOCK)
#include "AQS.h"
#elif defined(QSPIN_LOCK_OP)
#include "OpQSPINLOCK.h"
#elif defined(QSPINLOCK)
#include "QSPINLOCK.h"
#elif defined(MIX_LOCK_OP)
#include "OpMIX.h"
#endif

#ifdef OMCS_OFFSET
#if defined(OMCS_LOCK)
namespace offset {
using Lock = omcs_impl::OMCSLock;
using QNode = omcs_impl::OMCSQNode;
}  // namespace offset
namespace omcs_impl {
inline OMCSQNode *base_qnode = nullptr;
}  // namespace omcs_impl
#elif defined(MCSRW_LOCK) || defined(OPT_MCSRW_HYBRID_LOCK)
namespace offset {
using Lock = mcsrw::MCSRWLock;
using QNode = mcsrw::MCSRWQNode;
}  // namespace offset
namespace mcsrw {
inline MCSRWQNode *base_qnode = nullptr;
}  // namespace mcsrw
#elif defined(TC_LOCK_OP)
namespace offset {
using Lock = tc_lock::OpTCL;
using QNode = komb_node_t;
}  // namespace offset
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;
extern komb_node_t *komb_base_qnode;
#elif defined(OFP_LOCK)
namespace offset {
using Lock = tc_lock::OpFP;
using QNode = komb_node_t;
}  // namespace offset
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;
extern komb_node_t *komb_base_qnode;
#elif defined(OFP_MUTEX)
namespace offset {
using Lock = tc_lock::OpFPMutex;
using QNode = komb_node_t;
}  // namespace offset
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;
extern komb_node_t *komb_base_qnode;
#elif defined(TC_LOCK_NDL_OP)
namespace offset {
using Lock = tc_lock::OpTCL_NDL;
using QNode = komb_node_t;
}  // namespace offset
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;
extern komb_node_t *komb_base_qnode;
#elif defined(TC_LOCK)
namespace offset {
using Lock = tc_lock::TCLock;
using QNode = komb_node_t;
}  // namespace offset
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;
extern komb_node_t *komb_base_qnode;
#elif defined(TC_LOCK_RW)
namespace offset {
using Lock = tc_lock::TCLockRW;
using QNode = komb_node_t;
}  // namespace offset
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;
extern komb_node_t *komb_base_qnode;
extern thread_local uint8_t aqs_node_offset;
extern thread_local aqs_node_t *my_local_aqs_node;
extern aqs_node_t *aqs_base_qnode;
#elif defined(TC_UNLOCK_OP)
namespace offset {
using Lock = tc_unlock::OpTCLUnlock;
using QNode = komb_node_t;
}  // namespace offset
extern thread_local unsigned int komb_node_offset;
extern thread_local komb_node_t *my_local_komb_node;
extern komb_node_t *komb_base_qnode;
#elif defined(TD_LOCK_OP)
namespace offset {
using Lock = td_lock::OpTDL;
using QNode = kombd_node_t;
}  // namespace offset
extern thread_local unsigned int kombd_node_offset;
extern thread_local kombd_node_t *my_local_kombd_node;
extern kombd_node_t *kombd_base_qnode;
#elif defined(MIX_LOCK_OP)
namespace offset {
using Lock = mix_lock::OpMIX;
using QNode = mix_node_t;
}  // namespace offset
extern thread_local unsigned int mix_node_offset;
extern thread_local mix_node_t *my_local_mix_node;
extern mix_node_t *mix_base_qnode;
#elif defined(AQS_LOCK_OP)
namespace offset {
using Lock = aqs_lock::OpAQS;
using QNode = aqs_node_t;
} //namespace offset
extern thread_local uint8_t aqs_node_offset;
extern thread_local aqs_node_t *my_local_aqs_node;
extern aqs_node_t *aqs_base_qnode;
#elif defined(AQS_LOCK)
namespace offset {
using Lock = aqs_lock::AQS;
using QNode = aqs_node_t;
} //namespace offset
extern thread_local uint8_t aqs_node_offset;
extern thread_local aqs_node_t *my_local_aqs_node;
extern aqs_node_t *aqs_base_qnode;
#elif defined(QSPIN_LOCK_OP)
namespace offset {
using Lock = qspin_lock::OpQSPINLOCK;
using QNode = qspinlock_node_t;
} //namespace offset
extern thread_local unsigned int qspinlock_node_offset;
extern thread_local qspinlock_node_t *my_local_qspinlock_node;
extern qspinlock_node_t *qspinlock_base_qnode;
#elif defined(QSPINLOCK)
namespace offset {
using Lock = qspin_lock::QSpinLock;
using QNode = qspinlock_node_t;
} //namespace offset
extern thread_local unsigned int qspinlock_node_offset;
extern thread_local qspinlock_node_t *my_local_qspinlock_node;
extern qspinlock_node_t *qspinlock_base_qnode;
#endif
#endif //OMCS_OFFSET

#if defined(TC_LOCK_OP) || defined(TC_LOCK) || defined(TC_LOCK_RW) || defined(TC_LOCK_NDL_OP) || defined(OFP_LOCK) || defined(OFP_MUTEX)
#define TC_LOCK_VARIANT
#endif

#if defined(AQS_LOCK_OP) || defined(QSPIN_LOCK_OP) || defined(MIX_LOCK_OP) || defined(TD_LOCK_OP) || defined(TC_UNLOCK_OP) || defined(QSPINLOCK) || defined(AQS_LOCK) || defined(TC_LOCK_VARIANT)
#define CUSTOM_LOCK
#endif


namespace offset {
#ifdef OMCS_OFFSET
#if defined(OMCS_LOCK)
using omcs_impl::base_qnode;
#elif defined(MCSRW_LOCK) || defined(OPT_MCSRW_HYBRID_LOCK)
using mcsrw::base_qnode;
#elif defined(TC_LOCK_VARIANT)
using tc_lock::base_qnode;
#elif defined(TC_UNLOCK_OP)
using tc_unlock::base_qnode;
#elif defined(TD_LOCK_OP)
using td_lock::base_qnode;
#elif defined(MIX_LOCK_OP)
using mix_lock::base_qnode;
#elif defined(AQS_LOCK_OP) || defined(AQS_LOCK)
using aqs_lock::base_qnode;
#elif defined(QSPIN_LOCK_OP) || defined(QSPINLOCK)
using qspin_lock::base_qnode;
#endif
#if  defined(OMCS_OFFSET_NUMA_QNODE) || defined(CUSTOM_LOCK)
#define PAGE_SIZE 4096
struct socket_queue_node_index {
  std::atomic<uint64_t> index;
  char padding[CACHELINE_SIZE - sizeof(index)];
};
inline socket_queue_node_index *socket_qnode_index;
#else
inline std::atomic<uint64_t> next_node(0);
#endif  // OMCS_OFFSET_NUMA_QNODE
#endif  // OMCS_OFFSET

inline void tls_reset_index() {
#if defined(OMCS_OFFSET_NUMA_QNODE) || defined(CUSTOM_LOCK)
  uint32_t sockets = numa_max_node() + 1;
  for (uint32_t i = 0; i < sockets; ++i) {
    socket_qnode_index[i].index = 0;
  }
#endif
}

inline void init_qnodes() {
#ifdef OMCS_OFFSET
#if defined(OMCS_OFFSET_NUMA_QNODE) || defined(CUSTOM_LOCK)
  static_assert(PAGE_SIZE % sizeof(QNode) == 0);

  // Round up to the proper number of pages
  uint32_t qnodes_per_page = PAGE_SIZE / sizeof(QNode);
  uint32_t npages = 0;
  uint32_t qnodes = 0;
  uint32_t sockets = numa_max_node() + 1;
  while (qnodes < Lock::kNumQueueNodes) {
    qnodes += qnodes_per_page * sockets;
    npages += sockets;
  }

  std::cout << "Allocated " << qnodes << " queue nodes over " << npages << " pages across "
            << sockets << " sockets" << std::endl;

  socket_qnode_index = (socket_queue_node_index *)malloc(sockets * sizeof(socket_queue_node_index));
  for (uint32_t i = 0; i < sockets; ++i) {
    socket_qnode_index[i].index = 0;
  }

  base_qnode = (QNode *)numa_alloc_interleaved(npages * PAGE_SIZE);
  //base_qnode = (QNode *)malloc(npages * PAGE_SIZE);
  if (!base_qnode) {
    abort();
  }

#if defined(TC_LOCK_VARIANT) || defined(TC_UNLOCK_OP)
  komb_base_qnode = base_qnode;
#elif defined(AQS_LOCK_OP) || defined(AQS_LOCK) || defined(TC_LOCK_RW)
  aqs_base_qnode = base_qnode;
#elif defined(QSPIN_LOCK_OP) || defined(QSPINLOCK)
  qspinlock_base_qnode = base_qnode;
#elif defined(MIX_LOCK_OP)
  mix_base_qnode = base_qnode;
#elif defined(TD_LOCK_OP)
  kombd_base_qnode = base_qnode;
#endif
#else
  // int ret = posix_memalign((void **)&base_qnode, CACHELINE_SIZE * 2,
  //                          sizeof(QNode) * Lock::kNumQueueNodes);
  // if (ret) {
  //   abort();
  // }
  int node = 0;
  base_qnode = (QNode *)numa_alloc_onnode(sizeof(QNode) * Lock::kNumQueueNodes, node);

  for (uint32_t i = 0; i < Lock::kNumQueueNodes; ++i) {
    new (base_qnode + i) QNode;
  }
#endif  // OMCS_OFFSET_NUMA_QNODE
#endif  // OMCS_OFFSET
  return;
}

#ifdef OMCS_OFFSET
inline thread_local QNode *qnodes = nullptr;

inline QNode *get_qnode(size_t i) {
  assert(qnodes);
#if defined(CUSTOM_LOCK)
  return &qnodes[i];
#else
  new (&qnodes[i]) QNode;
  return &qnodes[i];
#endif
}
#endif

inline void reset_tls_qnodes() {
#ifdef OMCS_OFFSET
#ifdef BTREE_RWLOCK_MCSRW_ONLY
  // XXX(shiges): we might need 8-9 qnodes for our workloads
  constexpr size_t QNODES_PER_THREAD = 16;
#elif defined(CUSTOM_LOCK)
  constexpr size_t QNODES_PER_THREAD = 1;
#else
  // XXX(shiges): grab 4 qnodes every time
  constexpr size_t QNODES_PER_THREAD = 4;
#endif
#ifdef OMCS_OFFSET_NUMA_QNODE
  uint32_t socket = numa_node_of_cpu(sched_getcpu());
  //uint32_t qnodes_per_page = PAGE_SIZE / sizeof(QNode);
  //uint32_t index = socket_qnode_index[socket].index.fetch_add(QNODES_PER_THREAD);
  uint32_t index = socket_qnode_index[0].index.fetch_add(QNODES_PER_THREAD);
  //uint32_t nsockets = numa_max_node() + 1;
  //uint32_t page_num = index / qnodes_per_page * nsockets + socket;
  //index = page_num * qnodes_per_page + index % qnodes_per_page;
  //printf("omcsoffset socket: %d index: %d cpuid: %d\n", socket, index, sched_getcpu());
  qnodes = &offset::base_qnode[index];
#if defined(TC_LOCK_VARIANT) || defined(TC_UNLOCK_OP)
  komb_node_offset = index;
  my_local_komb_node = get_qnode(0);
#if defined(OFP_MUTEX)
  uint64_t cpuid = sched_getcpu();
  if(thread_to_node_mapping[cpuid] == 0) {
    thread_to_node_mapping[cpuid] = komb_node_offset;
    my_local_komb_node->is_combiner_running = false;
  } 
#endif
#elif defined(AQS_LOCK_OP) || defined(AQS_LOCK) || defined(TC_LOCK_RW)
  aqs_node_offset = index;
  my_local_aqs_node = get_qnode(0);
#elif defined(QSPIN_LOCK_OP) || defined(QSPINLOCK)
  qspinlock_node_offset = index;
  my_local_qspinlock_node = get_qnode(0);
#elif defined(MIX_LOCK_OP)
  mix_node_offset = index;
  my_local_mix_node = get_qnode(0);
#elif defined(TD_LOCK_OP)
  kombd_node_offset = index;
  my_local_kombd_node = get_qnode(0);
#endif
#else
  uint32_t index = next_node.fetch_add(QNODES_PER_THREAD);
  qnodes = &offset::base_qnode[index];
#endif  // OMCS_OFFSET_NUMA_QNODE
#endif
}
}  // namespace offset
