#pragma once

#include <glog/logging.h>

#if defined(LOCK_READER_STATS)
#include "timing_stats.h"
#endif

#if defined(TC_LOCK_OP)
#include "kombop.h"
#elif defined(OFP_LOCK)
#include "fpop.h"
#elif defined(OFP_MUTEX)
#include "fpop_mutex.h"
#elif defined(TC_LOCK_NDL_OP)
#include "kombop_ndl.h"
#elif defined(TC_LOCK)
#include "komb.h"
#elif defined(TC_LOCK_RW)
#include "kombrw.h"
#elif defined(TC_UNLOCK_OP)
#include "kombunlockop.h"
#elif defined(AQS_LOCK_OP)
#include "aqsop.h"
#elif defined(AQS_LOCK)
#include "aqs.h"
#elif defined(QSPIN_LOCK_OP)
#include "qspinlockop.h"
#elif defined(QSPINLOCK)
#include "qspinlock.h"
#elif defined(MIX_LOCK_OP)
#include "mixop.h"
#elif defined(TD_LOCK_OP)
#include "tdlockop.h"
#endif
thread_local unsigned int cur_thread_id = -1;

#if defined(TC_LOCK_OP) || defined(TC_LOCK_NDL_OP) || defined(TC_LOCK) || defined(TC_LOCK_RW) || defined(TC_UNLOCK_OP) || defined(OFP_LOCK) || defined(OFP_MUTEX)
#define TC_LOCK_VARIANT_WRAPPER
#endif

#if defined(TC_LOCK_VARIANT_WRAPPER)
extern long komb_batch_size;
#elif defined(TD_LOCK_OP)
extern long kombd_batch_size;
extern long kombd_num_threads_per_socket;
#endif

#include "stats.h"

#include "latches/OMCSOffset.h"
#if defined(BTREE_OL_CENTRALIZED)
#include "indexes/BTreeOLC/BTreeOLC.h"
using BTree = btreeolc::BTreeOLC<uint64_t, uint64_t>;
#elif defined(BTREE_OLC_UPGRADE)
#include "indexes/BTreeOLC/BTreeOLCNB.h"
using BTree = btreeolc::BTreeOLC<uint64_t, uint64_t>;
#elif defined(BTREE_OMCS_LEAF_ONLY)
#include "indexes/BTreeOLC/BTreeOMCSLeaf.h"
using BTree = btreeolc::BTreeOMCSLeaf<uint64_t, uint64_t>;
#elif defined(BTREE_OMCS_ALL)
#include "indexes/BTreeOLC/BTreeOMCS.h"
using BTree = btreeolc::BTreeOMCS<uint64_t, uint64_t>;
#elif defined(BTREE_RWLOCK)
#include "indexes/BTreeOLC/BTreeLC.h"
using BTree = btreeolc::BTreeLC<uint64_t, uint64_t>;
#elif defined(BTREE_RWLOCK_MCSRW_ONLY)
#include "indexes/BTreeOLC/BTreeLCMCSRWOnly.h"
using BTree = btreeolc::BTreeLC<uint64_t, uint64_t>;
#elif defined(BTREE_OLC_HYBRID)
#include "indexes/BTreeOLC/BTreeOLCHybrid.h"
using BTree = btreeolc::BTreeOLCHybrid<uint64_t, uint64_t>;
#elif defined(BTREE_OFP)
#include "indexes/BTreeOLC/BTreeOFP.h"
using BTree = btreeolc::BTreeOFP<uint64_t, uint64_t>;
#else
#error "BTree synchronization implementation is not defined."
#endif

#include "tree_api.hpp"

bool run_timers = false;
bool count_reads = false;

class btreeolc_wrapper : public tree_api {
 public:
  btreeolc_wrapper(const tree_options_t &opt);
  virtual ~btreeolc_wrapper();

  virtual bool bulk_load(const char *data, size_t num_records, size_t key_sz,
                         size_t value_sz) override final;
  virtual bool find(const char *key, size_t key_sz, char *value_out) override final;
  virtual bool insert(const char *key, size_t key_sz, const char *value,
                      size_t value_sz) override final;
  virtual bool update(const char *key, size_t key_sz, const char *value,
                      size_t value_sz) override final;
  virtual bool remove(const char *key, size_t key_sz) override final;
  virtual int scan(const char *key, size_t key_sz, int scan_sz, char *&values_out) override final;
  virtual void tls_setup() override final;
  virtual void tls_reset() override final;
  virtual void thread_timers_start() override final;
  virtual void thread_timers_stop() override final;
  virtual void thread_start(int thread_num) override final;
  virtual void thread_finish(int thread_num) override final;
  virtual void benchmark_start() override final;
  virtual void benchmark_finish() override final;
  virtual void set_combining_batch(uint32_t batch_size) override final;
  virtual void analyze_lock_contention() override final;
  virtual void run_delegation_thread(int* finished) override final;
  virtual void set_num_threads_per_socket(uint32_t num_threads_per_socket) override final;

 private:
  BTree *tree;
};

btreeolc_wrapper::btreeolc_wrapper(const tree_options_t &opt) {
  offset::init_qnodes();
  tree = new BTree();
}

btreeolc_wrapper::~btreeolc_wrapper() { 
#if defined(LOCK_READER_STATS)
  locktime_print_reader_stats();
#endif

  delete tree; 
}

bool btreeolc_wrapper::bulk_load(const char *data, size_t num_records, size_t key_sz,
                                 size_t value_sz) {
  // Fake bulk loading
  const char *pos = data;
  for (uint64_t i = 0; i < num_records; ++i) {
    uint64_t ikey = *reinterpret_cast<const uint64_t *>(pos);
    ikey = __builtin_bswap64(ikey);
    pos += key_sz;
    uint64_t ival = 0;
    memcpy(&ival, pos, sizeof(uint64_t));
    pos += value_sz;
    bool ok = tree->insert(ikey, ival);
    if (!ok) {
      return false;
    }
  }
  return true;
}

bool btreeolc_wrapper::find(const char *key, size_t key_sz, char *value_out) {
  uint64_t ikey = *reinterpret_cast<const uint64_t *>(key);
  ikey = __builtin_bswap64(ikey);
  uint64_t ival = 0;
  bool ok = tree->lookup(ikey, ival);
  *reinterpret_cast<uint64_t *>(value_out) = ival;
  return ok;
}

bool btreeolc_wrapper::insert(const char *key, size_t key_sz, const char *value, size_t value_sz) {
  uint64_t ikey = *reinterpret_cast<const uint64_t *>(key);
  ikey = __builtin_bswap64(ikey);
  uint64_t ival = 0;
  memcpy(&ival, value, sizeof(uint64_t));
  return tree->insert(ikey, ival);
}

bool btreeolc_wrapper::update(const char *key, size_t key_sz, const char *value, size_t value_sz) {
  uint64_t ikey = *reinterpret_cast<const uint64_t *>(key);
  ikey = __builtin_bswap64(ikey);
  uint64_t ival = 0;
  memcpy(&ival, value, sizeof(uint64_t));
  return tree->update(ikey, ival);
}

bool btreeolc_wrapper::remove(const char *key, size_t key_sz) {
  uint64_t ikey = *reinterpret_cast<const uint64_t *>(key);
  ikey = __builtin_bswap64(ikey);
  return tree->remove(ikey);
}

int btreeolc_wrapper::scan(const char *key, size_t key_sz, int scan_sz, char *&values_out) {
  static thread_local uint64_t buffer[1 << 16];
  values_out = reinterpret_cast<char *>(buffer);
  uint64_t ikey = *reinterpret_cast<const uint64_t *>(key);
  ikey = __builtin_bswap64(ikey);
  return tree->scan(ikey, scan_sz, buffer);
}

void btreeolc_wrapper::tls_reset() {
  offset::tls_reset_index();
};


void btreeolc_wrapper::tls_setup() {
  // XXX(shiges): hack
  offset::reset_tls_qnodes();
}

#include <unordered_map>
#include <unordered_set>
#include <mutex>

#if defined(MIX_LOCK_OP)
extern thread_local std::unordered_map<uint64_t, uint64_t> locks_seen;
static std::unordered_map<uint64_t, std::pair<uint64_t, std::unordered_set<uint64_t>>> all_locks_seen;
static std::mutex locks_seen_mutex;
#endif

void btreeolc_wrapper::thread_start(int thread_num) {
#if defined(TC_LOCK_VARIANT_WRAPPER) || defined(AQS_LOCK_OP) || defined(QSPIN_LOCK_OP) || defined(MIX_LOCK_OP) || defined(TD_LOCK_OP) || defined(QSPINLOCK) || defined(AQS_LOCK)
  cur_thread_id = thread_num;
#endif
#if defined(TC_LOCK_VARIANT_WRAPPER)
  komb_thread_start();
#elif defined(AQS_LOCK_OP) || defined(AQS_LOCK)
  aqs_thread_start();
#elif defined(MIX_LOCK_OP)
  mix_thread_start();
#elif defined(TD_LOCK_OP)
  kombd_thread_start();
#endif
  reset_thread_stats(thread_num);
#if defined(MIX_LOCK_OP)
	locks_seen.clear();
#endif
}

void btreeolc_wrapper::thread_finish(int thread_num) {
  aggregate_my_stats(thread_num);
#if defined(MIX_LOCK_OP)
	uint64_t lock_count = 0;
	std::lock_guard<std::mutex> guard(locks_seen_mutex);
	for(auto it: locks_seen) {
		all_locks_seen[it.first].first += it.second;
		all_locks_seen[it.first].second.insert(thread_num);
		if(it.second > 1000)
			lock_count++;
	}	
	std::cout << "Thread " << thread_num << ": " << locks_seen.size() << " Locks count>1000: " << lock_count << std::endl;
	locks_seen.clear();	
#endif
}

void btreeolc_wrapper::benchmark_start() {
  reset_aggregate_stats();
#if defined(MIX_LOCK_OP)
	std::lock_guard<std::mutex> guard(locks_seen_mutex);
	all_locks_seen.clear();
#endif
}

void btreeolc_wrapper::benchmark_finish() {
  print_aggregate_stats();
#if defined(MIX_LOCK_OP)
	#define NUM_CUTOFF 6
	uint64_t lock_count[NUM_CUTOFF] = {0};
	uint64_t thread_g_4[NUM_CUTOFF] = {0};
	uint64_t thread_le_4[NUM_CUTOFF] = {0};
	uint64_t cutoff[NUM_CUTOFF] = {100000, 10000, 1000, 100, 10, 0};
	for(auto it: all_locks_seen) {
		for(int i = 0; i < NUM_CUTOFF; i++) {
			if(it.second.first > cutoff[i]) {
				lock_count[i]++;
				if(it.second.second.size() > 4)
					thread_g_4[i]++;
				else
					thread_le_4[i]++;
				break;
			}
		}
	}
	std::cout << "Locks Seen size: " << all_locks_seen.size() << std::endl;
	for(int i = 0; i < NUM_CUTOFF; i++)
		std::cout << "Num lock count >" << cutoff[i] << ": " << lock_count[i] << " thread_g_4: " << thread_g_4[i] << " thread_le_4: " << thread_le_4[i] <<  std::endl;
#endif
}

void btreeolc_wrapper::analyze_lock_contention() {
#if defined(MIX_LOCK_OP)
  uint64_t num_switched_locks = 0;
  for(auto it: all_locks_seen) {
    if(it.second.first < 1000 || it.second.second.size() < 4) {
      ((mix_lock::OpMIX*)it.first)->disable_tclock();
      num_switched_locks++;
    }
  }
  std::cout << "Switched " << num_switched_locks << " locks to QSPINLOCK. Total: " << all_locks_seen.size() << std::endl;
  extern bool metric_collection;
  metric_collection = false;
#endif
}

void btreeolc_wrapper::thread_timers_start() {
#if defined(LOCK_READER_STATS)
  count_reads = true;
#endif
}

void btreeolc_wrapper::thread_timers_stop() {
#if defined(LOCK_READER_STATS)
  count_reads = false;
  locktime_add_reader_stats();
#endif
}

void btreeolc_wrapper::set_combining_batch(uint32_t batch_size) {
#if defined(TC_LOCK_VARIANT_WRAPPER)
  if(batch_size != 0)
    komb_batch_size = batch_size;
  std::cout << "OpTCL batch size : " << komb_batch_size << std::endl;
#elif defined(TD_LOCK_OP)
  if(batch_size != 0)
    kombd_batch_size = batch_size;
   std::cout << "OpTDL batch size : " << kombd_batch_size << std::endl; 
#endif
}

void btreeolc_wrapper::set_num_threads_per_socket(uint32_t num_threads_per_socket) {
#if defined(TD_LOCK_OP)
  kombd_num_threads_per_socket = num_threads_per_socket;
#endif
}

void btreeolc_wrapper::run_delegation_thread(int* finished) {
  #if defined(TD_LOCK_OP)
    komb_delegation_thread(finished);
  #endif
}
