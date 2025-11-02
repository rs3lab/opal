#pragma once

#include <atomic>
#include <cstdint>

#if defined(OMCS_LOCK)
#include "OMCSImpl.h"
#elif defined(STD_LOCK)
#include "STD.h"
#elif defined(STDRW_LOCK)
#include "STDRW.h"
#elif defined(TC_LOCK)
#include "TCL.h"
#elif defined(TC_LOCK_RW)
#include "TCLRW.h"
#elif defined(TC_LOCK_OP)
#include "OpTCL.h"
#elif defined(OFP_LOCK)
#include "OpFP.h"
#elif defined(OFP_MUTEX)
#include "OpFPMutex.h"
#elif defined(TC_LOCK_NDL_OP)
#include "OpTCL_NDL.h"
#elif defined(TC_UNLOCK_OP)
#include "OpTCLUnlock.h"
#elif defined(TD_LOCK_OP)
#include "OpTDL.h"
#elif defined(AQS_LOCK_OP)
#include "OpAQS.h"
#elif defined(QSPIN_LOCK_OP)
#include "OpQSPINLOCK.h"
#elif defined(MIX_LOCK_OP)
#include "OpMIX.h"
#elif defined(MCSRW_LOCK)
#include "MCSRW.h"
#elif defined(QSPINLOCK)
#include "QSPINLOCK.h"
#elif defined(AQS_LOCK)
#include "AQS.h"
#elif defined(OPT_MCSRW_HYBRID_LOCK)
#include "MCSRW.h"
#include "OMCSImpl.h"
#endif

#ifndef CACHELINE_SIZE
#define CACHELINE_SIZE 64
#endif

#include "timing_stats.h"
#if defined(LOCK_MEASURE_TIME)
LOCK_EXTERN_TIMING_VAR(write_cs);
#endif

struct OMCSLock {
#if defined(OMCS_LOCK)
  using Lock = omcs_impl::OMCSLock;
  using Context = omcs_impl::OMCSQNode;
  static constexpr uint64_t kInvalidVersion = omcs_impl::OMCSLock::kInvalidVersion;
  static constexpr const char *name = "OMCS Lock";
#elif defined(STD_LOCK)
  using Lock = std_lock::STDLock;
  using Context = uint64_t;
  static constexpr const char *name = "STD Mutex";
#elif defined(STDRW_LOCK)
  using Lock = std_lock::STDRWLock;
  using Context = uint64_t;
  static constexpr const char *name = "STD Shared Mutex";
#elif defined(TC_LOCK)
  using Lock = tc_lock::TCLock;
  using Context = komb_node_t;
  static constexpr uint64_t kInvalidVersion = tc_lock::TCLock::invalid_version;
  static constexpr const char *name = "Pessimistic TCLock";
#elif defined(TC_LOCK_RW)
  using Lock = tc_lock::TCLockRW;
  using Context = komb_node_t;
  static constexpr uint64_t kInvalidVersion = tc_lock::TCLockRW::invalid_version;
  static constexpr const char *name = "Pessimistic RW TCLock";
#elif defined(TC_LOCK_OP)
  using Lock = tc_lock::OpTCL;
  using Context = uint64_t;
  static constexpr uint64_t kInvalidVersion = tc_lock::OpTCL::invalid_version;
  static constexpr const char *name = "Optimistic TCLock";
#elif defined(OFP_LOCK)
  using Lock = tc_lock::OpFP;
  using Context = komb_node_t;
  static constexpr uint64_t kInvalidVersion = tc_lock::OpFP::invalid_version;
  static constexpr const char *name = "Optimistic TCLock without TC";
#elif defined(OFP_MUTEX)
  using Lock = tc_lock::OpFPMutex;
  using Context = komb_node_t;
  static constexpr uint64_t kInvalidVersion = tc_lock::OpFPMutex::invalid_version;
  static constexpr const char *name = "Optimistic TCLock without TC Mutex";
#elif defined(TC_LOCK_NDL_OP)
  using Lock = tc_lock::OpTCL_NDL;
  using Context = uint64_t;
  static constexpr uint64_t kInvalidVersion = tc_lock::OpTCL_NDL::invalid_version;
  static constexpr const char *name = "Optimistic TCLock without dependent load";
#elif defined(TC_UNLOCK_OP)
  using Lock = tc_unlock::OpTCLUnlock;
  using Context = uint64_t;
  static constexpr uint64_t kInvalidVersion = tc_unlock::OpTCLUnlock::invalid_version;
  static constexpr const char *name = "Optimistic TCUnLock";
 #elif defined(TD_LOCK_OP)
  using Lock = td_lock::OpTDL;
  using Context = uint64_t;
  static constexpr uint64_t kInvalidVersion = td_lock::OpTDL::invalid_version;
  static constexpr const char *name = "Optimistic TDLock"; 
 #elif defined(MIX_LOCK_OP)
  using Lock = mix_lock::OpMIX;
  using Context = uint64_t;
  static constexpr uint64_t kInvalidVersion = mix_lock::OpMIX::invalid_version;
  static constexpr const char *name = "Optimistic TCLock + Qspinlock"; 
#elif defined(AQS_LOCK_OP)
  using Lock = aqs_lock::OpAQS;
  using Context = uint64_t;
  static constexpr uint64_t kInvalidVersion = aqs_lock::OpAQS::invalid_version;
  static constexpr const char *name = "Optimistic AQS";
#elif defined(QSPIN_LOCK_OP)
  using Lock = qspin_lock::OpQSPINLOCK;
  using Context = uint64_t;
  static constexpr uint64_t kInvalidVersion = qspin_lock::OpQSPINLOCK::invalid_version;
  static constexpr const char *name = "Optimistic Queued Spinlock";
#elif defined(MCSRW_LOCK)
  using Lock = mcsrw::MCSRWLock;
  using Context = mcsrw::MCSRWQNode;
  static constexpr const char *name = "MCS RW Lock";
#elif defined(QSPINLOCK)
  using Lock = qspin_lock::QSpinLock;
  using Context = qspinlock_node_t;
  static constexpr uint64_t kInvalidVersion = qspin_lock::QSpinLock::invalid_version;
  static constexpr const char *name = "Pessimistic QSpinLock";
#elif defined(AQS_LOCK)
  using Lock = aqs_lock::AQS;
  using Context = aqs_node_t;
  static constexpr uint64_t kInvalidVersion = aqs_lock::AQS::invalid_version;
  static constexpr const char *name = "Pessimistic AQS";
#elif defined(OPT_MCSRW_HYBRID_LOCK)
  using Lock = uint64_t;
  using Context = mcsrw::MCSRWQNode;
  static constexpr uint64_t kInvalidVersion = omcs_impl::OMCSLock::kInvalidVersion;
  static constexpr const char *name = "OptLock + MCS RW Lock";
#else
#error "OMCS implementation is not defined."
#endif

#ifdef KOMB_PAD_LOCK
  alignas(CACHELINE_SIZE * 2) 
#endif
  Lock lock;

#if defined(OMCS_LOCK)
  // Optimistic lock API

  bool isLocked() const { return lock.is_locked(); }

  uint64_t readLock() const { return lock.begin_read(); }

  uint64_t readLockOrRestart(bool &needRestart) const { return lock.try_begin_read(needRestart); }

  uint64_t writeLock() { 
    auto res = lock.lock(); 
    LOCK_START_TIMING(write_cs);
    return res;
  }

  void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart) {
    // FIXME(shiges): update [version]
    needRestart = !lock.try_lock(version);
    if (!needRestart) {
      //LOCK_START_TIMING_PER_CPU(write_cs);
    }
  }

  void writeUnlock() {
    LOCK_END_TIMING(write_cs);
    return lock.unlock(); 
  }

  void writeUnlock(uint64_t version) {
    LOCK_END_TIMING(write_cs);
    return lock.unlock(version); 
  }

  bool writeLockBegin(Context *q) {
      auto res = lock.lock_begin(q); 
      LOCK_START_TIMING(write_cs);
      return res;
    }

  void writeLockTurnOffOpRead() { lock.lock_turn_off_opread(); }

  void writeLock(Context *q) {
    lock.lock(q); 
      LOCK_START_TIMING(write_cs);
    }

#if defined(OMCS_OP_READ_NEW_API_CALLBACK)
  template <class Callback>
  void writeLockWithRead(Context *q, Callback &&cb) {
    lock.lock(q, std::forward<Callback &&>(cb));
  }
#endif

  void upgradeToWriteLockOrRestart(uint64_t &version, Context *q, bool &needRestart) {
    // FIXME(shiges): update [version]
    needRestart = !lock.try_lock(q, version);
    if (!needRestart) {
      //LOCK_START_TIMING_PER_CPU(write_cs);
    }
  }

  void writeUnlock(Context *q) { 
    LOCK_END_TIMING(write_cs);
    return lock.unlock(q); }

  void checkOrRestart(uint64_t startRead, bool &needRestart) const {
    readUnlockOrRestart(startRead, needRestart);
  }

  void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const {
    needRestart = !lock.validate_read(startRead);
  }
#elif defined(TC_LOCK_OP) || defined(AQS_LOCK_OP) || defined(QSPIN_LOCK_OP) || defined(MIX_LOCK_OP) || defined(TD_LOCK_OP) || defined(TC_UNLOCK_OP) || defined(TC_LOCK_NDL_OP)
  // Optimistic lock API

  bool isLocked() const { 
    return lock.is_locked();
   }

  uint64_t readLock() const { 
    return lock.begin_read();
   }

  uint64_t readLockOrRestart(bool &needRestart) const { 
    return lock.try_begin_read(needRestart);  
  }

  uint64_t writeLock() { 
      lock.write_lock();
      //LOCK_START_TIMING(write_cs);
      return 0;
  }

  void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart) {
    needRestart = !lock.try_write_lock();
    if (!needRestart) {
      //LOCK_START_TIMING_PER_CPU(write_cs);
    }
  }

  void writeUnlock() { 
    //LOCK_END_TIMING(write_cs);
    lock.write_unlock(); 
  }

  void writeUnlock(uint64_t version) { 
    //LOCK_END_TIMING(write_cs);
    
    lock.write_unlock(); }

  bool writeLockBegin(Context *q) {
    std::cerr << "writeLockBegin Not supported" << std::endl;
    exit(0);
  }

  void writeLockTurnOffOpRead() {
    // LOG(FATAL) << "Not supported";
    // lock.turn_off_opt_reads();
  }

  void writeLock(Context *q) { 
      lock.write_lock(); 
      //LOCK_START_TIMING(write_cs);
    }

  void upgradeToWriteLockOrRestart(uint64_t &version, Context *q, bool &needRestart) {
    needRestart = !lock.try_write_lock();
  }

  void writeUnlock(Context *q) { 
    //LOCK_END_TIMING(write_cs);
    lock.write_unlock(); 
  }

  void checkOrRestart(uint64_t startRead, bool &needRestart) const {
    readUnlockOrRestart(startRead, needRestart);
  }

  void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const {
    lock.read_unlock_or_restart(startRead, needRestart);
  }

#elif defined(RWLOCK)
  // Readers-writer lock API

  void writeLock() { lock.lock(); }

  void writeUnlock() { lock.unlock(); }

  void readLock() { lock.read_lock(); }

  void readUnlock() { lock.read_unlock(); }

  void writeLock(Context *q) { lock.lock(q); }

  void writeUnlock(Context *q) { lock.unlock(q); }

  void readLock(Context *q) { lock.read_lock(q); }

  void readUnlock(Context *q) { lock.read_unlock(q); }

  // Dummy APIs
  uint64_t readLockOrRestart(bool &needRestart) const { LOG(FATAL) << "Not supported"; }

  void checkOrRestart(uint64_t startRead, bool &needRestart) const {
    LOG(FATAL) << "Not supported";
  }

  void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const {
    LOG(FATAL) << "Not supported";
  }

#elif defined(OFP_LOCK) || defined(OFP_MUTEX)
  // Readers-writer lock API

  bool isLocked() const {
    return lock.is_locked();
  }

  uint64_t readLock() const {
    return lock.begin_read();
  }

  uint64_t writeLock() { lock.write_lock(); return 0;}

  void writeUnlock() { lock.write_unlock(); }

  void writeUnlock(uint64_t version) { lock.write_unlock(); }

  void writeLock(Context *q) { lock.write_lock(); }

  void writeUnlock(Context *q) { lock.write_unlock(); }

  uint64_t readLockOrRestart(bool &needRestart) const { 
    return lock.try_begin_read(needRestart);
  }

  void checkOrRestart(uint64_t startRead, bool &needRestart) const {
    readUnlockOrRestart(startRead, needRestart);
  }

  void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const {
    lock.read_unlock_or_restart(startRead, needRestart);
  }

  void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart) {
    //needRestart = !lock.try_write_lock();
    needRestart = !lock.try_write_lock(version);
  }

  void upgradeToWriteLockOrRestart(uint64_t &version, Context *q, bool &needRestart) {
    needRestart = !lock.try_write_lock();
  }

  void writeLockTurnOffOpRead() {
    lock.turn_off_opt_reads();
  }

#if defined(ART_OFP_LOCK)
  int executeARTOp(void *k, uint64_t new_tid, void *node, void* parentNode, uint64_t v, int (*op)(void*, uint64_t, void*, void*, uint64_t)) {
    return lock.execute(k, new_tid, node, parentNode, v, op);
  }
#else 
  virtual bool update(uint64_t k, uint64_t p) = 0;

  static int updateOperation(void *node, uint64_t versionNode, void *next, uint64_t k, uint64_t v) {
    auto casted_node = static_cast<OMCSLock *>(node);
    bool needRestart = false;
    casted_node->readUnlockOrRestart(versionNode, needRestart);
    if(needRestart)
      return -1;
    auto casted_next = static_cast<OMCSLock *>(next);
    bool ok = casted_next->update(k, v);
    return ok;
  }

  int executeOperation(void *node, uint64_t versionNode, void *next, uint64_t k, uint64_t v) {
    return lock.execute(node, versionNode, next, k, v, &updateOperation);
  }
#endif // ART_OFP_LOCK

#endif
};

#if defined(OPT_MCSRW_HYBRID_LOCK)
#if not defined(OMCS_OFFSET)
static_assert(false, "OMCS_OFFSET must be defined for OptLock + MCS RW Lock hybrid");
#endif

struct OptLock {
  omcs_impl::OMCSLock lock;

  // Optimistic lock API
  uint64_t readLockOrRestart(bool &needRestart) const { return lock.try_begin_read(needRestart); }

  void checkOrRestart(uint64_t startRead, bool &needRestart) const {
    readUnlockOrRestart(startRead, needRestart);
  }

  void readUnlockOrRestart(uint64_t startRead, bool &needRestart) const {
    needRestart = !lock.validate_read(startRead);
  }

  void upgradeToWriteLockOrRestart(uint64_t &version, bool &needRestart) {
    // FIXME(shiges): update [version]
    needRestart = !lock.try_lock(version);
  }

  void writeUnlock(uint64_t version) { return lock.unlock(version); }
};

struct MCSRWLock {
  mcsrw::MCSRWLock lock;

  // Readers-writer lock API
  void writeLock(OMCSLock::Context *q) { lock.lock(q); }

  void writeUnlock(OMCSLock::Context *q) { lock.unlock(q); }

  void readLock(OMCSLock::Context *q) { lock.read_lock(q); }

  void readUnlock(OMCSLock::Context *q) { lock.read_unlock(q); }
};
#endif

#if defined(OMCS_LOCK)
static_assert(sizeof(OMCSLock) == 8, "sizeof OMCSLock is not 8-byte");
#elif defined(STDRW_LOCK)
static_assert(sizeof(OMCSLock) == 56, "sizeof OMCSLock is not 56-byte");
#elif defined(MCSRW_LOCK)
#if defined(OMCS_OFFSET)
static_assert(sizeof(OMCSLock) == 8, "sizeof OMCSLock is not 8-byte");
#endif
#elif defined(OPT_MCSRW_HYBRID_LOCK)
static_assert(sizeof(OptLock) == 8, "sizeof OptLock is not 8-byte");
static_assert(sizeof(MCSRWLock) == 8, "sizeof MCSRWLock is not 8-byte");
#endif
