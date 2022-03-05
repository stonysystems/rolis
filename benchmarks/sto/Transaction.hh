#pragma once

#include "config.h"
#include "compiler.hh"
#include "small_vector.hh"
#include "TRcu.hh"
#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <map>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <cstring>
#include <unordered_map>
#include <x86intrin.h>
#include <atomic>


#ifndef STO_PROFILE_COUNTERS
#define STO_PROFILE_COUNTERS 0
#endif

#ifndef STO_DEBUG_HASH_COLLISIONS
#define STO_DEBUG_HASH_COLLISIONS 0
#endif
#ifndef STO_DEBUG_HASH_COLLISIONS_FRACTION
#define STO_DEBUG_HASH_COLLISIONS_FRACTION 0.00001
#endif

#ifndef STO_DEBUG_ABORTS
#define STO_DEBUG_ABORTS 0
#endif
#ifndef STO_DEBUG_ABORTS_FRACTION
#define STO_DEBUG_ABORTS_FRACTION 0.0001
#endif

#ifndef STO_SORT_WRITESET
#define STO_SORT_WRITESET 0
#endif

#ifndef STO_SPIN_EXPBACKOFF
#define STO_SPIN_EXPBACKOFF 0
#endif

#ifndef STO_SPIN_BOUND_WRITE
#if STO_SPIN_EXPBACKOFF
#define STO_SPIN_BOUND_WRITE 7
#else
#define STO_SPIN_BOUND_WRITE 3
#endif
#endif

#ifndef STO_SPIN_BOUND_WAIT
#if STO_SPIN_EXPBACKOFF
#define STO_SPIN_BOUND_WAIT 18
#else
#define STO_SPIN_BOUND_WAIT 20
#endif
#endif

#define CONSISTENCY_CHECK 0
#define ASSERT_TX_SIZE 0
#define TRANSACTION_HASHTABLE 1

#if ASSERT_TX_SIZE
#if STO_PROFILE_COUNTERS > 1
#    define TX_SIZE_LIMIT 20000
#    include <cassert>
#else
#    error "ASSERT_TX_SIZE requires STO_PROFILE_COUNTERS > 1!"
#endif
#endif

#include "config.h"

#define MAX_THREADS_T 256

// weihshen, 4098*4098*3 should be enough, rather than 4098*4098*1024
#define MAX_ARRAY_SIZE_IN_BYTES  size_t(4096)*size_t(3)*size_t(4096)*sizeof(char)

struct PUnit{
  unsigned long long int cid;
  int length;
  int tsize;
  const unsigned char *start;
};

void register_sync_util(std::function<int()>);

class SiloBatching{
 private:
  unsigned char *LOG;
  size_t total_alloc_size;
  size_t _offset;
  size_t actual_len;
  
  size_t kSizeLimit;
  std::vector<PUnit> _prequalVector;
  size_t entries;
  size_t curr_pos;
  
 public:
  
  explicit SiloBatching(int batchLimitSize, size_t max_mem_allowed=MAX_ARRAY_SIZE_IN_BYTES){
    int prequalPayloadLen = sizeof (PUnit)*batchLimitSize;
    _offset = sizeof (batchLimitSize) + sizeof (curr_pos) + prequalPayloadLen;
    total_alloc_size = _offset + max_mem_allowed;
    LOG = (unsigned char *) malloc (total_alloc_size);
    memset(LOG,'\0',total_alloc_size);
    curr_pos = _offset;
    entries = 0;
    actual_len = 0;
    kSizeLimit = batchLimitSize;
  }
  
  ~SiloBatching ();
  
  bool checkPushRequired(){
	return entries>=kSizeLimit;   // XXX, weihshen TODO: the limit of buffer size or timeout
  }
  
  void pushIfNeeded(bool force=false);
  
  void resetMemory(){
    curr_pos = _offset;
    entries = 0;
    actual_len = 0;
  	_prequalVector.clear ();
  	memset(LOG,'\0',total_alloc_size);
  }
  
  unsigned char *getBatchedLog(size_t &lpos){
    size_t pos = 0;
    lpos = 0;
	int vector_count = this->_prequalVector.size ();
	if (vector_count == 0)
	  return nullptr;
	int lag =0;
	int prequalPayloadLen = sizeof (PUnit) * vector_count;
	if(kSizeLimit != this->_prequalVector.size ())
	{
	  lag = _offset-prequalPayloadLen-sizeof (vector_count)-sizeof (actual_len);
	  pos = lag;
	}
	
	unsigned char *ptr = LOG;

	// current ptr is at 0th location
	// copy number of commit IDS->int
	memcpy ((void *) (ptr + pos), &vector_count, sizeof (vector_count));
	pos += sizeof (vector_count);
	lpos += sizeof (vector_count);

	// copy curr_pos->size_t
	memcpy ((void *) (ptr + pos), &actual_len, sizeof (actual_len));
	pos += sizeof (actual_len);
	lpos += sizeof (actual_len);

	// copy all data from vector
	memcpy ((void *) (ptr + pos), this->_prequalVector.data (), prequalPayloadLen);
	pos += prequalPayloadLen;
	lpos += prequalPayloadLen;
	assert (pos == _offset);

	// optimization : no need to copy actual log since it is already written at correct position
	//	memcpy ((void *) (ptr + pos), LOG + _offset, actual_len);
	pos += actual_len;
	lpos += actual_len;
	
	assert(pos == curr_pos);
	return ptr+lag;
  }
  
  // always return start of the address
  unsigned char * getLogOnly(size_t& pos){
    pos = curr_pos;
	return LOG;
  }
  
  /*
   * check if new data size logical write is allowed
   */
  bool CheckIfOutofMemLimits (size_t newLogLen){
  	return (curr_pos + newLogLen) >= total_alloc_size;
  }

  /*
   * Logically update the pointer as if new memory are being copied
   * incr the entries
   */
  void update_ptr (const unsigned long long int& _cid, // commit ID
  	              int _len,							   // log length
  	              const unsigned char **startAddr,     // current log buffer start address
  	              int _tsize)
  {
    _prequalVector.emplace_back (PUnit{.cid=_cid,
									   .length=_len,
									   .tsize=_tsize,
									   .start=*startAddr});
    entries++;
    curr_pos += _len;
    actual_len += _len;
  }
};

class StringAllocator{
 private:
  unsigned char *LOG;
  size_t ul_len = sizeof (unsigned long long int);
  
  void freeMemory(){
    resetMemory();
    std::cout << "Mem Free Now" << std::endl;
    ::free(LOG);
  }
  
 public:
  size_t entries;
  size_t curr_pos;
  unsigned long long int latest_commit_id;
  
  StringAllocator(){
    LOG = (unsigned char *) malloc (MAX_ARRAY_SIZE_IN_BYTES);
    //memset(LOG,'\0',MAX_ARRAY_SIZE_IN_BYTES);
    curr_pos = 0;
    entries = 0;
    latest_commit_id = 0;
  }
  ~StringAllocator();

  static size_t kSizeLimit;
  static void setSTOBatchSize(size_t k){
    kSizeLimit = k;
  }
  bool checkPushRequired(){
	return entries>=kSizeLimit;
  }
  unsigned char * getLog(int size=0){
    if(size==0)
		return LOG;
    else{
	  return LOG+curr_pos-size;
    }
  }
  unsigned char * getLogOnly(size_t& pos){
    pos = curr_pos;
	return LOG;
  }
  
  void resetMemory(){
    curr_pos = 0;
    entries = 0;
    latest_commit_id = 0;
  }
  bool checkLimits(size_t newLogLen){
  	return (curr_pos + newLogLen) < MAX_ARRAY_SIZE_IN_BYTES;
  }
  
  inline void bypass_add (const unsigned long long int& value) {
	memcpy (LOG + curr_pos, (char *) &value, ul_len);
	curr_pos+=ul_len;
  }
  
  inline void increment () {
	entries++;
  }
  
  inline void update_ptr(const size_t& w){
    entries++;
    curr_pos=w;
  }

  inline void update_commit_id(const unsigned long long int & nid) {
      latest_commit_id=nid;
  }
  
  bool add (const char *array, int len) {
	if(!checkLimits(len))
	{
	  std::cout << "[]ERROR <<<<< No more memory in the string pool " << std::endl;
	  return false;
	}
	entries++;
  //  std::lock_guard<std::mutex> guard(paxos_lock);
	memcpy (LOG+curr_pos,array,len);
	curr_pos+=len;
	return true;
  }
};

class Buffer{
  public:
  static std::string LOG_0;
  static std::string LOG_1;
  
  static void add(const std::string& array,int pid);
};
class TimerMapper{
 public:
  static std::unordered_map<std::string,long double> timer;
  static void add_time(const std::string&, long double, long double);
};

class LogGenRate{
 public:
  static std::unordered_map<long int,std::pair<long int, long int>> counter;
  static void increment(long int& ts, size_t& w){
    auto old = counter[ts];
    old.first +=1;
    old.second +=w;
    counter[ts] = old;
  }
};

inline std::atomic<long> trans_cnt = 0;
inline std::unordered_map<std::string, long> ref_output ;
inline std::mutex mtx={};
inline std::mutex mtx2={};
inline std::atomic<long> len_payment(0);
inline std::atomic<long> len_new_order(0);
inline std::atomic<long> len_delivery(0);
inline std::function<void(const char*, int, int)> paxos_intercept ;
inline std::unordered_map<unsigned long long int, uint64_t> sample_transaction_tracker ;  // cid => updated time

// TRANSACTION macros that can be used to wrap transactional code
#define TRANSACTION                               \
    do {                                          \
        TransactionLoopGuard __txn_guard;         \
        while (1) {                               \
            __txn_guard.start();                  \
            try {
#define RETRY(retry)                              \
                if (__txn_guard.try_commit())     \
                    break;                        \
            } catch (Transaction::Abort e) {      \
            }                                     \
            if (!(retry))                         \
                throw Transaction::Abort();       \
        }                                         \
    } while (0)
    
// TRANSACTION macros that can be used to wrap transactional code

#define NO_PAXOS_TRANSACTION                               \
    do {                                          \
        TransactionLoopGuard __txn_guard;         \
        while (1) {                               \
            __txn_guard.start();                  \
            try {
#define NO_PAXOS_RETRY(retry)                              \
			if (__txn_guard.try_commit_nopxs())     \
                    break;                        \
            } catch (Transaction::Abort e) {      \
            }                                     \
            if (!(retry))                         \
                throw Transaction::Abort();       \
        }                                         \
    } while (0)

// transaction performance counters
enum txp {
    // all logging levels
    txp_total_aborts = 0,
    txp_total_starts,
    txp_commit_time_nonopaque,
    txp_commit_time_aborts,
    txp_max_set,
    txp_hco,
    txp_hco_lock,
    txp_hco_invalid,
    txp_hco_abort,
    // STO_PROFILE_COUNTERS > 1 only
    txp_total_n,
    txp_total_r,
    txp_total_w,
    txp_max_transbuffer,
    txp_total_transbuffer,
    txp_push_abort,
    txp_pop_abort,
    txp_total_check_read,
    txp_total_check_predicate,
    txp_hash_find,
    txp_hash_collision,
    txp_hash_collision2,
    txp_total_searched,
#if !STO_PROFILE_COUNTERS
    txp_count = 0
#elif STO_PROFILE_COUNTERS == 1
    txp_count = txp_hco_abort + 1
#else
    txp_count
#endif
};
typedef uint64_t txp_counter_type;

inline constexpr bool txp_is_max(unsigned p) {
    return p == txp_max_set || p == txp_max_transbuffer;
}

template <unsigned P, unsigned N, bool Less = (P < N)> struct txp_helper;
template <unsigned P, unsigned N> struct txp_helper<P, N, true> {
    static bool counter_exists(unsigned p) {
        return p < N;
    }
    static void account_array(txp_counter_type* p, txp_counter_type v) {
        if (txp_is_max(P))
            p[P] = std::max(p[P], v);
        else
            p[P] += v;
    }
};
template <unsigned P, unsigned N> struct txp_helper<P, N, false> {
    static bool counter_exists(unsigned) {
        return false;
    }
    static void account_array(txp_counter_type*, txp_counter_type) {
    }
};

struct txp_counters {
    txp_counter_type p_[txp_count];
    txp_counters() {
        for (unsigned i = 0; i != txp_count; ++i)
            p_[i] = 0;
    }
    unsigned long long p(int p) {
        return txp_helper<0, txp_count>::counter_exists(p) ? p_[p] : 0;
    }
    void reset() {
        for (int i = 0; i != txp_count; ++i)
            p_[i] = 0;
    }
};


#include "Interface.hh"
#include "TransItem.hh"

void reportPerf();
#define STO_SHUTDOWN() reportPerf()

struct __attribute__((aligned(128))) threadinfo_t {
    using epoch_type = TRcuSet::epoch_type;
    epoch_type epoch;
    TRcuSet rcu_set;
    // XXX(NH): these should be vectors so multiple data structures can register
    // callbacks for these
    std::function<void(void)> trans_start_callback;
    std::function<void(void)> trans_end_callback;
    txp_counters p_;
    threadinfo_t()
        : epoch(0) {
    }
};


class Transaction {
public:
    static size_t submit_calls;
  	static size_t delete_inserted_count;
    static constexpr unsigned tset_initial_capacity = 512;

    static constexpr unsigned hash_size = 1024;
    static constexpr unsigned hash_step = 5;
    using epoch_type = TRcuSet::epoch_type;
    using signed_epoch_type = TRcuSet::signed_epoch_type;
    static thread_local unsigned char * per_thread_array;

    static threadinfo_t tinfo[MAX_THREADS_T];
    static struct epoch_state {
        epoch_type global_epoch; // != 0
        epoch_type active_epoch; // no thread is before this epoch
        TransactionTid::type recent_tid;
        bool run;
    } global_epochs;
    typedef TransactionTid::type tid_type;
private:
    static TransactionTid::type _TID;
public:

    static std::function<void(threadinfo_t::epoch_type)> epoch_advance_callback;

    static txp_counters txp_counters_combined() {
        txp_counters out;
        for (int i = 0; i != MAX_THREADS_T; ++i)
            for (int p = 0; p != txp_count; ++p) {
                if (txp_is_max(p))
                    out.p_[p] = std::max(out.p_[p], tinfo[i].p_.p_[p]);
                else
                    out.p_[p] += tinfo[i].p_.p_[p];
            }
        return out;
    }

    static void print_stats();

    static void clear_stats() {
        for (int i = 0; i != MAX_THREADS_T; ++i)
            tinfo[i].p_.reset();
    }

    static void* epoch_advancer(void*);
    template <typename T>
    static void rcu_delete(T* x) {
        auto& thr = tinfo[TThread::id()];
        thr.rcu_set.add(thr.epoch, ObjectDestroyer<T>::destroy_and_free, x);
    }
    template <typename T>
    static void rcu_delete_array(T* x) {
        auto& thr = tinfo[TThread::id()];
        thr.rcu_set.add(thr.epoch, ObjectDestroyer<T>::destroy_and_free_array, x);
    }
    static void rcu_free(void* ptr) {
        auto& thr = tinfo[TThread::id()];
        thr.rcu_set.add(thr.epoch, ::free, ptr);
    }
    static void rcu_call(void (*function)(void*), void* argument) {
        auto& thr = tinfo[TThread::id()];
        thr.rcu_set.add(thr.epoch, function, argument);
    }
    static void rcu_quiesce() {
        tinfo[TThread::id()].epoch = 0;
    }

#if STO_PROFILE_COUNTERS
    template <unsigned P> static void txp_account(txp_counter_type n) {
        txp_helper<P, txp_count>::account_array(tinfo[TThread::id()].p_.p_, n);
    }
#else
    template <unsigned P> static void txp_account(txp_counter_type) {
    }
#endif

#define TXP_INCREMENT(p) Transaction::txp_account<(p)>(1)
#define TXP_ACCOUNT(p, n) Transaction::txp_account<(p)>((n))


private:
    static constexpr unsigned tset_chunk = 512;
    static constexpr unsigned tset_max_capacity = 32768;

    void initialize();

    Transaction()
        : threadid_(TThread::id()), is_test_(false) {
        initialize();
        start();
    }

    struct testing_type {};
    static testing_type testing;

    Transaction(int threadid, const testing_type&)
        : threadid_(threadid), is_test_(true) {
        initialize();
        start();
    }

    Transaction(bool)
        : threadid_(TThread::id()), is_test_(false) {
        initialize();
        state_ = s_aborted;
    }

    ~Transaction();

    // reset data so we can be reused for another transaction
    void start() {
        threadinfo_t& thr = tinfo[TThread::id()];
        //if (isAborted_
        //   && tinfo[TThread::id()].p(txp_total_aborts) % 0x10000 == 0xFFFF)
           //print_stats();
        thr.epoch = global_epochs.global_epoch;
        thr.rcu_set.clean_until(global_epochs.active_epoch);
        if (thr.trans_start_callback)
            thr.trans_start_callback();
        hash_base_ += tset_size_ + 1;
        tset_size_ = 0;
        tset_next_ = tset0_;
#if TRANSACTION_HASHTABLE
        if (hash_base_ >= 32768) {
            memset(hashtable_, 0, sizeof(hashtable_));
            hash_base_ = 0;
        }
#endif
        any_writes_ = any_nonopaque_ = may_duplicate_items_ = false;
        first_write_ = 0;
        start_tid_ = commit_tid_ = 0;
	tid_unique_ = 0;
        buf_.clear();
#if STO_DEBUG_ABORTS
        abort_item_ = nullptr;
        abort_reason_ = nullptr;
        abort_version_ = 0;
#endif
        TXP_INCREMENT(txp_total_starts);
        state_ = s_in_progress;
    }

#if TRANSACTION_HASHTABLE
    static int hash(const TObject* obj, void* key) {
        auto n = reinterpret_cast<uintptr_t>(key) + 0x4000000;
        n += -uintptr_t(n < 0x8000000) & (reinterpret_cast<uintptr_t>(obj) >> 4);
        //2654435761
        return (n + (n >> 16) * 9) % hash_size;
    }
#endif

    void refresh_tset_chunk();

    TransItem* allocate_item(const TObject* obj, void* xkey) {
        if (tset_size_ && tset_size_ % tset_chunk == 0)
            refresh_tset_chunk();
        ++tset_size_;
        new(reinterpret_cast<void*>(tset_next_)) TransItem(const_cast<TObject*>(obj), xkey);
#if TRANSACTION_HASHTABLE
        unsigned hi = hash(obj, xkey);
# if TRANSACTION_HASHTABLE > 1
        if (hashtable_[hi] > hash_base_)
            hi = (hi + hash_step) % hash_size;
# endif
        if (hashtable_[hi] <= hash_base_)
            hashtable_[hi] = hash_base_ + tset_size_;
#endif
        return tset_next_++;
    }

public:
    int threadid() const {
        return threadid_;
    }

    // adds item for a key that is known to be new (must NOT exist in the set)
    template <typename T>
    TransProxy new_item(const TObject* obj, T key) {
        void* xkey = Packer<T>::pack_unique(buf_, std::move(key));
        return TransProxy(*this, *allocate_item(obj, xkey));
    }

    // adds item without checking its presence in the array
    template <typename T>
    TransProxy fresh_item(const TObject* obj, T key) {
        may_duplicate_items_ = tset_size_ > 0;
        void* xkey = Packer<T>::pack_unique(buf_, std::move(key));
        return TransProxy(*this, *allocate_item(obj, xkey));
    }

    // tries to find an existing item with this key, otherwise adds it
    template <typename T>
    TransProxy item(const TObject* obj, T key) {
        void* xkey = Packer<T>::pack_unique(buf_, std::move(key));
        TransItem* ti = find_item(const_cast<TObject*>(obj), xkey);
        if (!ti)
            ti = allocate_item(obj, xkey);
        return TransProxy(*this, *ti);
    }

    // gets an item that is intended to be read only. this method essentially allows for duplicate items
    // in the set in some cases
    template <typename T>
    TransProxy read_item(const TObject* obj, T key) {
        void* xkey = Packer<T>::pack_unique(buf_, std::move(key));
        TransItem* ti = nullptr;
        if (any_writes_)
            ti = find_item(const_cast<TObject*>(obj), xkey);
        else
            may_duplicate_items_ = tset_size_ > 0;
        if (!ti)
            ti = allocate_item(obj, xkey);
        return TransProxy(*this, *ti);
    }

    template <typename T>
    OptionalTransProxy check_item(const TObject* obj, T key) const {
        void* xkey = Packer<T>::pack_unique(buf_, std::move(key));
        TransItem* ti = find_item(const_cast<TObject*>(obj), xkey);
        return OptionalTransProxy(const_cast<Transaction&>(*this), ti);
    }

private:
    // tries to find an existing item with this key, returns NULL if not found
    TransItem* find_item(TObject* obj, void* xkey) const {
#if TRANSACTION_HASHTABLE
        TXP_INCREMENT(txp_hash_find);
        unsigned hi = hash(obj, xkey);
        for (int steps = 0; steps < TRANSACTION_HASHTABLE; ++steps) {
            if (hashtable_[hi] <= hash_base_)
                return nullptr;
            unsigned tidx = hashtable_[hi] - hash_base_ - 1;
            const TransItem* ti;
            if (likely(tidx < tset_initial_capacity))
                ti = &tset0_[tidx];
            else
                ti = &tset_[tidx / tset_chunk][tidx % tset_chunk];
            if (ti->owner() == obj && ti->key_ == xkey)
                return const_cast<TransItem*>(ti);
            if (!steps) {
                TXP_INCREMENT(txp_hash_collision);
# if STO_DEBUG_HASH_COLLISIONS
                if (local_random() <= uint32_t(0xFFFFFFFF * STO_DEBUG_HASH_COLLISIONS_FRACTION)) {
                    std::ostringstream buf;
                    TransItem fake_item(obj, xkey);
                    buf << "$ STO hash collision: search " << fake_item << ", find " << *ti << '\n';
                    std::cerr << buf.str();
                }
# endif
            } else
                TXP_INCREMENT(txp_hash_collision2);
            hi = (hi + hash_step) % hash_size;
        }
#endif
        const TransItem* it = nullptr;
        for (unsigned tidx = 0; tidx != tset_size_; ++tidx) {
            it = (tidx % tset_chunk ? it + 1 : tset_[tidx / tset_chunk]);
            TXP_INCREMENT(txp_total_searched);
            if (it->owner() == obj && it->key_ == xkey)
                return const_cast<TransItem*>(it);
        }
        return nullptr;
    }

    bool preceding_duplicate_read(TransItem *it) const;

#if STO_DEBUG_ABORTS
    void mark_abort_because(TransItem* item, const char* reason, TVersion::type version = 0) const {
        abort_item_ = item;
        abort_reason_ = reason;
        if (version)
            abort_version_ = version;
    }
#else
    void mark_abort_because(TransItem*, const char*, TVersion::type = 0) const {
    }
#endif

    void abort_because(TransItem& item, const char* reason, TVersion::type version = 0) {
        mark_abort_because(&item, reason, version);
        abort();
    }

public:
    void silent_abort() {
        if (in_progress())
            stop(false, nullptr, 0);
    }

    void abort() {
        silent_abort();
        throw Abort();
    }

    bool try_commit(bool no_paxos= false);

    void commit() {
        if (!try_commit())
            throw Abort();
    }

    bool aborted() {
        return state_ == s_aborted;
    }

    bool in_progress() {
        return state_ < s_aborted;
    }

    // opacity checking
    // These function will eventually help us track the commit TID when we
    // have no opacity, or for GV7 opacity.
    bool try_lock(TransItem& item, TVersion& vers) {
        return try_lock(item, const_cast<TransactionTid::type&>(vers.value()));
    }
    bool try_lock(TransItem& item, TNonopaqueVersion& vers) {
        return try_lock(item, const_cast<TransactionTid::type&>(vers.value()));
    }
    bool try_lock(TransItem& item, TransactionTid::type& vers) {
#if STO_SORT_WRITESET
        (void) item;
        TransactionTid::lock(vers, threadid_);
        return true;
#else
        // This function will eventually help us track the commit TID when we
        // have no opacity, or for GV7 opacity.
        unsigned n = 0;
        while (1) {
            if (TransactionTid::try_lock(vers, threadid_))
                return true;
            ++n;
# if STO_SPIN_EXPBACKOFF
            if (item.has_read() || n == STO_SPIN_BOUND_WRITE) {
#  if STO_DEBUG_ABORTS
                abort_version_ = vers;
#  endif
                return false;
            }
            if (n > 3)
                for (unsigned x = 1 << std::min(15U, n - 2); x; --x)
                    relax_fence();
# else
            if (item.has_read() || n == (1 << STO_SPIN_BOUND_WRITE)) {
#  if STO_DEBUG_ABORTS
                abort_version_ = vers;
#  endif
                return false;
            }
# endif
            relax_fence();
        }
#endif
    }

    void check_opacity(TransItem& item, TransactionTid::type v) {
        assert(state_ <= s_committing_locked);
        if (!start_tid_)
            start_tid_ = _TID;
        if (!TransactionTid::try_check_opacity(start_tid_, v)
            && state_ < s_committing)
            hard_check_opacity(&item, v);
    }
    void check_opacity(TransItem& item, TVersion v) {
        check_opacity(item, v.value());
    }
    void check_opacity(TransItem&, TNonopaqueVersion) {
    }

    void check_opacity(TransactionTid::type v) {
        assert(state_ <= s_committing_locked);
        if (!start_tid_)
            start_tid_ = _TID;
        if (!TransactionTid::try_check_opacity(start_tid_, v)
            && state_ < s_committing)
            hard_check_opacity(nullptr, v);
    }

    void check_opacity() {
        check_opacity(_TID);
    }

    // committing
    tid_type commit_tid() const {
        assert(state_ == s_committing_locked || state_ == s_committing);
        if (!commit_tid_){
            commit_tid_ = fetch_and_add(&_TID, TransactionTid::increment_value);
	    tid_unique();
	}
        return commit_tid_;
    }
    
    uint64_t tid_unique() const {
	    assert(state_ == s_committing_locked || state_ == s_committing);
	    unsigned int ui;
	    if(!tid_unique_)
		    tid_unique_ = __rdtscp(&ui);
	   return tid_unique_;
    }

    void set_version(TVersion& vers, TVersion::type flags = 0) const {
        vers.set_version(commit_tid() | flags);
    }
    void set_version_unlock(TVersion& vers, TransItem& item, TVersion::type flags = 0) const {
        vers.set_version_unlock(commit_tid() | flags);
        item.clear_needs_unlock();
    }
    void assign_version_unlock(TVersion& vers, TransItem& item, TVersion::type flags = 0) const {
        vers = commit_tid() | flags;
        item.clear_needs_unlock();
    }
    void set_version(TNonopaqueVersion& vers, TNonopaqueVersion::type flags = 0) const {
        assert(state_ == s_committing_locked || state_ == s_committing);
        tid_type v = commit_tid_ ? commit_tid_ : TransactionTid::next_unflagged_nonopaque_version(vers.value());
        vers.set_version(v | flags);
    }
    void set_version_unlock(TNonopaqueVersion& vers, TransItem& item, TNonopaqueVersion::type flags = 0) const {
        assert(state_ == s_committing_locked || state_ == s_committing);
        tid_type v = commit_tid_ ? commit_tid_ : TransactionTid::next_unflagged_nonopaque_version(vers.value());
        vers.set_version_unlock(v | flags);
        item.clear_needs_unlock();
    }
    void assign_version_unlock(TNonopaqueVersion& vers, TransItem& item, TNonopaqueVersion::type flags = 0) const {
        tid_type v = commit_tid_ ? commit_tid_ : TransactionTid::next_unflagged_nonopaque_version(vers.value());
        vers = v | flags;
        item.clear_needs_unlock();
    }

    static const char* state_name(int state);
    void print() const;
    inline void serialize_util(unsigned nwriteset) const;
    void print(std::ostream& w) const;

    class Abort {};

    uint32_t local_random() const {
        lrng_state_ = lrng_state_ * 1664525 + 1013904223;
        return lrng_state_;
    }
    void local_srandom(uint32_t state) {
        lrng_state_ = state;
    }
    
    static bool hasDeleteOp(const TransItem* item) {
      return item->flags() & TransItem::mdelete_bit;
  	}
    
    bool hasInsertOp(const TransItem* item) const{
      return item->has_flag (TransItem::minsert_bit);
  	}

private:
    enum {
        s_in_progress = 0, s_opacity_check = 1, s_committing = 2,
        s_committing_locked = 3, s_aborted = 4, s_committed = 5
    };

    int threadid_;
    uint16_t hash_base_;
    uint16_t first_write_;
    uint8_t state_;
    bool any_writes_;
    bool any_nonopaque_;
    bool may_duplicate_items_;
    bool is_test_;
    TransItem* tset_next_;
    unsigned tset_size_;
    mutable tid_type start_tid_;
    mutable tid_type commit_tid_;
    mutable TransactionBuffer buf_;
    mutable uint64_t tid_unique_; // using __rdtscp instructio
    mutable uint32_t lrng_state_;
    
    static constexpr TransItem::flags_type insert_bit_modified = TransItem::user0_bit;
    static constexpr TransItem::flags_type delete_bit_modified = TransItem::user0_bit<<1;
    
#if STO_DEBUG_ABORTS
    mutable TransItem* abort_item_;
    mutable const char* abort_reason_;
    mutable TVersion::type abort_version_;
#endif
    TransItem* tset_[tset_max_capacity / tset_chunk];
#if TRANSACTION_HASHTABLE
    uint16_t hashtable_[hash_size];
#endif
    TransItem tset0_[tset_initial_capacity];

    void hard_check_opacity(TransItem* item, TransactionTid::type t);
    void stop(bool committed, unsigned* writes, unsigned nwrites);

    friend class TransProxy;
    friend class TransItem;
    friend class Sto;
    friend class TestTransaction;
    friend class TNonopaqueVersion;
};


class Sto {
public:
    static Transaction* transaction() {
        if (!TThread::txn)
            TThread::txn = new Transaction(false);
        return TThread::txn;
    }

    static void start_transaction() {
        Transaction* t = transaction();
        always_assert(!t->in_progress());
        t->start();
    }

    static void update_threadid() {
        if (TThread::txn)
            TThread::txn->threadid_ = TThread::id();
    }

    static bool in_progress() {
        return TThread::txn && TThread::txn->in_progress();
    }

    static void abort() {
        always_assert(in_progress());
        TThread::txn->abort();
    }

    static void silent_abort() {
        if (in_progress())
            TThread::txn->silent_abort();
    }

    template <typename T>
    static TransProxy item(const TObject* s, T key) {
        always_assert(in_progress());
        return TThread::txn->item(s, key);
    }

    static void check_opacity(TransactionTid::type t) {
        always_assert(in_progress());
        TThread::txn->check_opacity(t);
    }

    static void check_opacity() {
        always_assert(in_progress());
        TThread::txn->check_opacity();
    }

    template <typename T>
    static OptionalTransProxy check_item(const TObject* s, T key) {
        always_assert(in_progress());
        return TThread::txn->check_item(s, key);
    }

    template <typename T>
    static TransProxy new_item(const TObject* s, T key) {
        always_assert(in_progress());
        return TThread::txn->new_item(s, key);
    }

    template <typename T>
    static TransProxy read_item(const TObject* s, T key) {
        always_assert(in_progress());
        return TThread::txn->read_item(s, key);
    }

    template <typename T>
    static TransProxy fresh_item(const TObject* s, T key) {
        always_assert(in_progress());
        return TThread::txn->fresh_item(s, key);
    }

    static void commit() {
        always_assert(in_progress());
        TThread::txn->commit();
    }

    static bool try_commit() {
        always_assert(in_progress());
        return TThread::txn->try_commit();
    }

    static bool try_commit_no_paxos() {
        always_assert(in_progress());
        return TThread::txn->try_commit(true);
    }

    static TransactionTid::type commit_tid() {
        return TThread::txn->commit_tid();
    }

    static TransactionTid::type recent_tid() {
        return Transaction::global_epochs.recent_tid;
    }

    static TransactionTid::type initialized_tid() {
        // XXX: we might want a nonopaque_bit in here too.
        return TransactionTid::increment_value;
    }
};

class TestTransaction {
public:
    TestTransaction(int threadid)
        : t_(threadid, Transaction::testing), base_(TThread::txn) {
        use();
    }
    ~TestTransaction() {
        if (base_ && !base_->is_test_) {
            TThread::txn = base_;
            TThread::set_id(base_->threadid_);
        }
    }
    void use() {
        TThread::txn = &t_;
        TThread::set_id(t_.threadid_);
    }
    void print(std::ostream& w) const {
        t_.print(w);
    }
    bool try_commit() {
        use();
        return t_.try_commit();
    }
private:
    Transaction t_;
    Transaction* base_;
};

class TransactionGuard {
  public:
    TransactionGuard() {
        Sto::start_transaction();
    }
    ~TransactionGuard() {
        Sto::commit();
    }
    typedef void (TransactionGuard::* unspecified_bool_type)(std::ostream&) const;
    operator unspecified_bool_type() const {
        return &TransactionGuard::print;
    }
    void print(std::ostream& w) const {
        TThread::txn->print(w);
    }
};

class TransactionLoopGuard {
  public:
    TransactionLoopGuard() {
    }
    ~TransactionLoopGuard() {
        if (TThread::txn->in_progress())
            TThread::txn->silent_abort();
    }
    void start() {
        Sto::start_transaction();
    }
    bool try_commit() {
        return TThread::txn->try_commit();
    }
    
    bool try_commit_nopxs() {
        return TThread::txn->try_commit(true);
    }
    
};


template <typename T>
inline TransProxy& TransProxy::add_read(T rdata) {
    assert(!has_stash());
    if (!has_read()) {
        item().__or_flags(TransItem::read_bit);
        item().rdata_ = Packer<T>::pack(t()->buf_, std::move(rdata));
        t()->any_nonopaque_ = true;
    }
    return *this;
}

inline TransProxy& TransProxy::add_extra(std::string extra) {
    item().extra = extra ;
    return *this;
}

// like add_read but checks opacity too.
// should be used by data structures that have non-TransactionTid
// versions and still need to respect opacity.
template <typename T>
inline TransProxy& TransProxy::add_read_opaque(T rdata) {
    assert(!has_stash());
    t()->check_opacity();
    if (!has_read()) {
        item().__or_flags(TransItem::read_bit);
        item().rdata_ = Packer<T>::pack(t()->buf_, std::move(rdata));
    }
    return *this;
}

inline TransProxy& TransProxy::observe(TVersion version, bool add_read) {
    assert(!has_stash());
    if (version.is_locked_elsewhere(t()->threadid_))
        t()->abort_because(item(), "locked", version.value());
    t()->check_opacity(item(), version.value());
    if (add_read && !has_read()) {
        item().__or_flags(TransItem::read_bit);
        item().rdata_ = Packer<TVersion>::pack(t()->buf_, std::move(version));
    }
    return *this;
}

inline TransProxy& TransProxy::observe(TNonopaqueVersion version, bool add_read) {
    assert(!has_stash());
    if (version.is_locked_elsewhere(t()->threadid_))
        t()->abort_because(item(), "locked", version.value());
    if (add_read && !has_read()) {
        item().__or_flags(TransItem::read_bit);
        item().rdata_ = Packer<TNonopaqueVersion>::pack(t()->buf_, std::move(version));
        t()->any_nonopaque_ = true;
    }
    return *this;
}

inline TransProxy& TransProxy::observe(TCommutativeVersion version, bool add_read) {
    assert(!has_stash());
    if (version.is_locked())
        t()->abort_because(item(), "locked", version.value());
    t()->check_opacity(item(), version.value());
    if (add_read && !has_read()) {
        item().__or_flags(TransItem::read_bit);
        item().rdata_ = Packer<TCommutativeVersion>::pack(t()->buf_, std::move(version));
    }
    return *this;
}

inline TransProxy& TransProxy::observe(TVersion version) {
    return observe(version, true);
}

inline TransProxy& TransProxy::observe(TNonopaqueVersion version) {
    return observe(version, true);
}

inline TransProxy& TransProxy::observe(TCommutativeVersion version) {
    return observe(version, true);
}

inline TransProxy& TransProxy::observe_opacity(TVersion version) {
    return observe(version, false);
}

inline TransProxy& TransProxy::observe_opacity(TNonopaqueVersion version) {
    return observe(version, false);
}

inline TransProxy& TransProxy::observe_opacity(TCommutativeVersion version) {
    return observe(version, false);
}

template <typename T>
inline TransProxy& TransProxy::update_read(T old_rdata, T new_rdata) {
    if (has_read() && this->read_value<T>() == old_rdata)
        item().rdata_ = Packer<T>::repack(t()->buf_, item().rdata_, new_rdata);
    return *this;
}


inline TransProxy& TransProxy::set_predicate() {
    assert(!has_read());
    item().__or_flags(TransItem::predicate_bit);
    return *this;
}

template <typename T>
inline TransProxy& TransProxy::set_predicate(T pdata) {
    assert(!has_read());
    item().__or_flags(TransItem::predicate_bit);
    item().rdata_ = Packer<T>::pack(t()->buf_, std::move(pdata));
    return *this;
}

template <typename T>
inline T& TransProxy::predicate_value(T default_pdata) {
    assert(!has_read());
    if (!has_predicate())
        set_predicate(default_pdata);
    return this->template predicate_value<T>();
}

inline TransProxy& TransProxy::add_write() {
    if (!has_write()) {
        item().__or_flags(TransItem::write_bit);
        t()->any_writes_ = true;
    }
    return *this;
}

template <typename T>
inline TransProxy& TransProxy::add_write(const T& wdata) {
    return add_write<T, const T&>(wdata);
}

template <typename T>
inline TransProxy& TransProxy::add_write(T&& wdata) {
    typedef typename std::decay<T>::type V;
    return add_write<V, V&&>(std::move(wdata));
}

template <typename T, typename... Args>
inline TransProxy& TransProxy::add_write(Args&&... args) {
    if (!has_write()) {
        item().__or_flags(TransItem::write_bit);
        item().wdata_ = Packer<T>::pack(t()->buf_, std::forward<Args>(args)...);
        t()->any_writes_ = true;
    } else
        // TODO: this assumes that a given writer data always has the same type.
        // this is certainly true now but we probably shouldn't assume this in general
        // (hopefully we'll have a system that can automatically call destructors and such
        // which will make our lives much easier)
        item().wdata_ = Packer<T>::repack(t()->buf_, item().wdata_, std::forward<Args>(args)...);
    return *this;
}

template <typename T>
inline TransProxy& TransProxy::set_stash(T sdata) {
    assert(!has_read());
    if (!has_stash()) {
        item().__or_flags(TransItem::stash_bit);
        item().rdata_ = Packer<T>::pack(t()->buf_, std::move(sdata));
    } else
        item().rdata_ = Packer<T>::repack(t()->buf_, item().rdata_, std::move(sdata));
    return *this;
}

template <typename Exception>
inline void TNonopaqueVersion::opaque_throw(const Exception&) {
    Sto::abort();
}

inline auto TVersion::snapshot(TransProxy& item) -> type {
    type v = value();
    item.observe_opacity(TVersion(v));
    return v;
}

inline auto TVersion::snapshot(const TransItem& item, const Transaction& txn) -> type {
    type v = value();
    const_cast<Transaction&>(txn).check_opacity(const_cast<TransItem&>(item), v);
    return v;
}

inline auto TNonopaqueVersion::snapshot(TransProxy& item) -> type {
    item.transaction().any_nonopaque_ = true;
    return value();
}

inline auto TNonopaqueVersion::snapshot(const TransItem&, const Transaction& txn) -> type {
    const_cast<Transaction&>(txn).any_nonopaque_ = true;
    return value();
}

inline bool TVersion::is_locked_here(const Transaction& txn) const {
    return is_locked_here(txn.threadid());
}

inline bool TNonopaqueVersion::is_locked_here(const Transaction& txn) const {
    return is_locked_here(txn.threadid());
}

inline bool TVersion::is_locked_elsewhere(const Transaction& txn) const {
    return is_locked_elsewhere(txn.threadid());
}

inline bool TNonopaqueVersion::is_locked_elsewhere(const Transaction& txn) const {
    return is_locked_elsewhere(txn.threadid());
}

std::ostream& operator<<(std::ostream& w, const Transaction& txn);
std::ostream& operator<<(std::ostream& w, const TestTransaction& txn);
std::ostream& operator<<(std::ostream& w, const TransactionGuard& txn);
