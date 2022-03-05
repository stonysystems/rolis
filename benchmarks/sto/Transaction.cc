#include "Transaction.hh"
#include <typeinfo>
#include <set>
#include "OutputDataSerializer.h"
//#include "SerializeUtility.h"
#include "Hashtable.hh"
#include "MassTrans.hh"
#include "Interface.hh"

#include "deptran/s_main.h"

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <queue>
#include <mutex>


std::mutex paxos_lock{};

size_t StringAllocator::kSizeLimit = 1000;
std::function<int()> callback_ = nullptr;
//size_t SiloBatching::kSizeLimit=100;

uint64_t
util_cur_usec() {
    struct timeval tv;
    gettimeofday(&tv, 0);
    return ((uint64_t) tv.tv_sec) * 1000000 + tv.tv_usec;
}

void register_sync_util(std::function<int()> cb) {
    callback_ = cb;
}

StringAllocator::~StringAllocator() {
#if !defined(SILO_SERIAL_ONLY)
    size_t pos = 0;
    unsigned char *queueLog = getLogOnly(pos);
#if defined(PAXOS_LIB_ENABLED)
    assert(pos <= MAX_ARRAY_SIZE_IN_BYTES) ;
      if(pos!=0) {
          assert(latest_commit_id > 0) ;
          // 7. latest_commit_id
          memcpy (queueLog + pos, (char *) &latest_commit_id, sizeof(unsigned long long int));
        pos += sizeof(unsigned long long int);
#ifdef ALLOW_PAXOS_INTERCEPT
          paxos_intercept((char *)queueLog, pos, TThread::getPartitionID ());
#else
        #if !defined(NO_ADD_LOG_TO_NC)
          add_log_to_nc((char *)queueLog, pos, TThread::getPartitionID ());
        #endif
#endif
      }
#endif
#endif
}

std::unordered_map<long int, std::pair<long int, long int>> LogGenRate::counter = {};
std::unordered_map<std::string, long double> TimerMapper::timer = {};

std::string Buffer::LOG_0 = "";
std::string Buffer::LOG_1 = "";

void TimerMapper::add_time(const std::string &key, long double value, long double denom) {
#ifdef BENCHMARK_TIME
    value /= denom;
    if(timer.find(key)==timer.end()){
      timer[key] = value;
    }else{
      timer[key]+=value;
    }
#endif
}

Transaction::testing_type Transaction::testing;
size_t Transaction::delete_inserted_count = 0;
size_t Transaction::submit_calls = 0;
thread_local unsigned char *Transaction::per_thread_array = NULL;
threadinfo_t Transaction::tinfo[MAX_THREADS_T];
__thread int TThread::the_id;
__thread int TThread::pid;
Transaction::epoch_state __attribute__((aligned(128))) Transaction::global_epochs = {
        1, 0, TransactionTid::increment_value, true
};
__thread Transaction *TThread::txn = nullptr;
std::function<void(threadinfo_t::epoch_type)> Transaction::epoch_advance_callback;
TransactionTid::type __attribute__((aligned(128))) Transaction::_TID = 2 * TransactionTid::increment_value;
// reserve TransactionTid::increment_value for prepopulated



static void __attribute__((used)) check_static_assertions() {
    static_assert(sizeof(threadinfo_t) % 128 == 0, "threadinfo is 2-cache-line aligned");
}

static std::set<uint64_t> sss;
static unsigned int abcsz;

void Transaction::initialize() {
    static_assert(tset_initial_capacity % tset_chunk == 0, "tset_initial_capacity not an even multiple of tset_chunk");
    hash_base_ = 32768;
    tset_size_ = 0;
    lrng_state_ = 12897;
    for (unsigned i = 0; i != tset_initial_capacity / tset_chunk; ++i)
        tset_[i] = &tset0_[i * tset_chunk];
    for (unsigned i = tset_initial_capacity / tset_chunk; i != arraysize(tset_); ++i)
        tset_[i] = nullptr;
}

Transaction::~Transaction() {
    if (in_progress())
        silent_abort();
    TransItem *live = tset0_;
    for (unsigned i = 0; i != arraysize(tset_); ++i, live += tset_chunk)
        if (live != tset_[i])
            delete[] tset_[i];
}

void Transaction::refresh_tset_chunk() {
    assert(tset_size_ % tset_chunk == 0);
    assert(tset_size_ < tset_max_capacity);
    if (!tset_[tset_size_ / tset_chunk])
        tset_[tset_size_ / tset_chunk] = new TransItem[tset_chunk];
    tset_next_ = tset_[tset_size_ / tset_chunk];
}

void *Transaction::epoch_advancer(void *) {
    static int num_epoch_advancers = 0;
    if (fetch_and_add(&num_epoch_advancers, 1) != 0)
        std::cerr << "WARNING: more than one epoch_advancer thread\n";

    // don't bother epoch'ing til things have picked up
    usleep(100000);
    while (global_epochs.run) {
        epoch_type g = global_epochs.global_epoch;
        epoch_type e = g;
        for (auto &t: tinfo) {
            if (t.epoch != 0 && signed_epoch_type(t.epoch - e) < 0)
                e = t.epoch;
        }
        global_epochs.global_epoch = std::max(g + 1, epoch_type(1));
        global_epochs.active_epoch = e;
        global_epochs.recent_tid = Transaction::_TID;

        if (epoch_advance_callback)
            epoch_advance_callback(global_epochs.global_epoch);

        usleep(100000);
    }
    fetch_and_add(&num_epoch_advancers, -1);
    return NULL;
}

bool Transaction::preceding_duplicate_read(TransItem *needle) const {
    const TransItem *it = nullptr;
    for (unsigned tidx = 0;; ++tidx) {
        it = (tidx % tset_chunk ? it + 1 : tset_[tidx / tset_chunk]);
        if (it == needle)
            return false;
        if (it->owner() == needle->owner() && it->key_ == needle->key_
            && it->has_read())
            return true;
    }
}

void Transaction::hard_check_opacity(TransItem *item, TransactionTid::type t) {
    // ignore opacity checks during commit; we're in the middle of checking
    // things anyway
    if (state_ == s_committing || state_ == s_committing_locked)
        return;

    // ignore if version hasn't changed
    if (item && item->has_read() && item->read_value<TransactionTid::type>() == t)
        return;

    // die on recursive opacity check; this is only possible for predicates
    if (unlikely(state_ == s_opacity_check)) {
        mark_abort_because(item, "recursive opacity check", t);
        abort:
        TXP_INCREMENT(txp_hco_abort);
        abort();
    }
    assert(state_ == s_in_progress);

    TXP_INCREMENT(txp_hco);
    if (TransactionTid::is_locked_elsewhere(t, threadid_)) {
        TXP_INCREMENT(txp_hco_lock);
        mark_abort_because(item, "locked", t);
        goto abort;
    }
    if (t & TransactionTid::nonopaque_bit)
        TXP_INCREMENT(txp_hco_invalid);

    state_ = s_opacity_check;
    start_tid_ = _TID;
    release_fence();
    TransItem *it = nullptr;
    for (unsigned tidx = 0; tidx != tset_size_; ++tidx) {
        it = (tidx % tset_chunk ? it + 1 : tset_[tidx / tset_chunk]);
        if (it->has_read()) {
            TXP_INCREMENT(txp_total_check_read);
            if (!it->owner()->check(*it, *this)
                && (!may_duplicate_items_ || !preceding_duplicate_read(it))) {
                mark_abort_because(item, "opacity check");
                goto abort;
            }
        } else if (it->has_predicate()) {
            TXP_INCREMENT(txp_total_check_predicate);
            if (!it->owner()->check_predicate(*it, *this, false)) {
                mark_abort_because(item, "opacity check_predicate");
                goto abort;
            }
        }
    }
    state_ = s_in_progress;
}

void Transaction::stop(bool committed, unsigned *writeset, unsigned nwriteset) {
    if (!committed) {
        TXP_INCREMENT(txp_total_aborts);
#if STO_DEBUG_ABORTS
        if (local_random() <= uint32_t(0xFFFFFFFF * STO_DEBUG_ABORTS_FRACTION)) {
            std::ostringstream buf;
            buf << "$" << (threadid_ < 10 ? "0" : "") << threadid_
                << " abort " << state_name(state_);
            if (abort_reason_)
                buf << " " << abort_reason_;
            if (abort_item_)
                buf << " " << *abort_item_;
            if (abort_version_)
                buf << " V" << TVersion(abort_version_);
            buf << '\n';
            std::cerr << buf.str();
        }
#endif
    }

    TXP_ACCOUNT(txp_max_transbuffer, buf_.size());
    TXP_ACCOUNT(txp_total_transbuffer, buf_.size());

    TransItem *it;
    if (!any_writes_)
        goto after_unlock;

    if (committed && !STO_SORT_WRITESET) {
        for (unsigned *idxit = writeset + nwriteset; idxit != writeset;) {
            --idxit;
            if (*idxit < tset_initial_capacity)
                it = &tset0_[*idxit];
            else
                it = &tset_[*idxit / tset_chunk][*idxit % tset_chunk];
            if (it->needs_unlock())
                it->owner()->unlock(*it);
        }
        for (unsigned *idxit = writeset + nwriteset; idxit != writeset;) {
            --idxit;
            if (*idxit < tset_initial_capacity)
                it = &tset0_[*idxit];
            else
                it = &tset_[*idxit / tset_chunk][*idxit % tset_chunk];
            if (it->has_write()) // always true unless a user turns it off in install()/check()
                it->owner()->cleanup(*it, committed);
        }
    } else {
        if (state_ == s_committing_locked) {
            it = &tset_[tset_size_ / tset_chunk][tset_size_ % tset_chunk];
            for (unsigned tidx = tset_size_; tidx != first_write_; --tidx) {
                it = (tidx % tset_chunk ? it - 1 : &tset_[(tidx - 1) / tset_chunk][tset_chunk - 1]);
                if (it->needs_unlock())
                    it->owner()->unlock(*it);
            }
        }
        it = &tset_[tset_size_ / tset_chunk][tset_size_ % tset_chunk];
        for (unsigned tidx = tset_size_; tidx != first_write_; --tidx) {
            it = (tidx % tset_chunk ? it - 1 : &tset_[(tidx - 1) / tset_chunk][tset_chunk - 1]);
            if (it->has_write())
                it->owner()->cleanup(*it, committed);
        }
    }

    after_unlock:
    // TODO: this will probably mess up with nested transactions
    threadinfo_t &thr = tinfo[TThread::id()];
    if (thr.trans_end_callback)
        thr.trans_end_callback();
    // XXX should reset trans_end_callback after calling it...
    state_ = s_aborted + committed;
}

bool Transaction::try_commit(bool no_paxos) {
    assert(TThread::id() == threadid_);
#if ASSERT_TX_SIZE
    if (tset_size_ > TX_SIZE_LIMIT) {
        std::cerr << "transSet_ size at " << tset_size_
            << ", abort." << std::endl;
        assert(false);
    }
#endif
    TXP_ACCOUNT(txp_max_set, tset_size_);
    TXP_ACCOUNT(txp_total_n, tset_size_);

    assert(state_ == s_in_progress || state_ >= s_aborted);
    if (state_ >= s_aborted)
        return state_ > s_aborted;

    if (any_nonopaque_)
        TXP_INCREMENT(txp_commit_time_nonopaque);
#if !CONSISTENCY_CHECK
    // commit immediately if read-only transaction with opacity
    if (!any_writes_ &&
        !any_nonopaque_) {  // XXX, read or snapshot => any_nonopaque_ = true, write => any_writes_ = true; no-read or no-snapshot => with opacity
        stop(true, nullptr, 0);
        return true;
    }
#endif

    state_ = s_committing;

    unsigned writeset[tset_size_];
    unsigned nwriteset = 0;
    writeset[0] = tset_size_;

    TransItem *it = nullptr;
    for (unsigned tidx = 0; tidx != tset_size_; ++tidx) {
        it = (tidx % tset_chunk ? it + 1 : tset_[tidx / tset_chunk]);
        if (it->has_write()) {
            writeset[nwriteset++] = tidx;
#if !STO_SORT_WRITESET
            if (nwriteset == 1) {
                first_write_ = writeset[0];
                state_ = s_committing_locked;
            }
            if (!it->owner()->lock(*it, *this)) {
                mark_abort_because(it, "commit lock");
                goto abort;
            }
            it->__or_flags(TransItem::lock_bit);
#endif
        }
        if (it->has_read())
            TXP_INCREMENT(txp_total_r);
        else if (it->has_predicate()) {
            TXP_INCREMENT(txp_total_check_predicate);
            if (!it->owner()->check_predicate(*it, *this, true)) {
                mark_abort_because(it, "commit check_predicate");
                goto abort;
            }
        }
    }

    first_write_ = writeset[0];

    //phase1
#if STO_SORT_WRITESET
    std::sort(writeset, writeset + nwriteset, [&] (unsigned i, unsigned j) {
        TransItem* ti = &tset_[i / tset_chunk][i % tset_chunk];
        TransItem* tj = &tset_[j / tset_chunk][j % tset_chunk];
        return *ti < *tj;
    });

    if (nwriteset) {
        state_ = s_committing_locked;
        auto writeset_end = writeset + nwriteset;
        for (auto it = writeset; it != writeset_end; ) {
            TransItem* me = &tset_[*it / tset_chunk][*it % tset_chunk];
            if (!me->owner()->lock(*me, *this)) {
                mark_abort_because(me, "commit lock");
                goto abort;
            }
            me->__or_flags(TransItem::lock_bit);
            ++it;
        }
    }
#endif


#if CONSISTENCY_CHECK
    fence();
    commit_tid(); //tid_unique();
    fence();
#endif

    //phase2
    for (unsigned tidx = 0; tidx != tset_size_; ++tidx) {
        it = (tidx % tset_chunk ? it + 1 : tset_[tidx / tset_chunk]);
        if (it->has_read()) {
            TXP_INCREMENT(txp_total_check_read);
            if (!it->owner()->check(*it, *this)
                && (!may_duplicate_items_ || !preceding_duplicate_read(it))) {
                mark_abort_because(it, "commit check");
                goto abort;
            }
        }
    }

    // fence();

    //phase3
#if STO_SORT_WRITESET
    for (unsigned tidx = first_write_; tidx != tset_size_; ++tidx) {
        it = &tset_[tidx / tset_chunk][tidx % tset_chunk];
        if (it->has_write()) {
            TXP_INCREMENT(txp_total_w);
            it->owner()->install(*it, *this);
        }
    }
#else
    if (nwriteset) {
        auto writeset_end = writeset + nwriteset;
        for (auto idxit = writeset; idxit != writeset_end; ++idxit) {
            if (likely(*idxit < tset_initial_capacity))
                it = &tset0_[*idxit];
            else
                it = &tset_[*idxit / tset_chunk][*idxit % tset_chunk];
            TXP_INCREMENT(txp_total_w);
            it->owner()->install(*it, *this);
        }
    }
#endif

    // fence();
#if defined(SERIALIZE_FEATURE)
    if(!no_paxos)
        serialize_util(nwriteset);
#endif
    stop(true, writeset, nwriteset);
    return true;

    abort:
    // fence();
    TXP_INCREMENT(txp_commit_time_aborts);
    stop(false, nullptr, 0);
    return false;
}

void Transaction::print_stats() {
    txp_counters out = txp_counters_combined();
    if (txp_count >= txp_max_set) {
        unsigned long long txc_total_starts = out.p(txp_total_starts);
        unsigned long long txc_total_aborts = out.p(txp_total_aborts);
        unsigned long long txc_commit_aborts = out.p(txp_commit_time_aborts);
        unsigned long long txc_total_commits = txc_total_starts - txc_total_aborts;
        fprintf(stderr, "$ %llu starts, %llu max read set, %llu commits",
                txc_total_starts, out.p(txp_max_set), txc_total_commits);
        if (txc_total_aborts) {
            fprintf(stderr, ", %llu (%.3f%%) aborts",
                    out.p(txp_total_aborts),
                    100.0 * (double) out.p(txp_total_aborts) / out.p(txp_total_starts));
            if (out.p(txp_commit_time_aborts))
                fprintf(stderr, "\n$ %llu (%.3f%%) of aborts at commit time",
                        out.p(txp_commit_time_aborts),
                        100.0 * (double) out.p(txp_commit_time_aborts) / out.p(txp_total_aborts));
        }
        unsigned long long txc_commit_attempts = txc_total_starts - (txc_total_aborts - txc_commit_aborts);
        fprintf(stderr, "\n$ %llu commit attempts, %llu (%.3f%%) nonopaque\n",
                txc_commit_attempts, out.p(txp_commit_time_nonopaque),
                100.0 * (double) out.p(txp_commit_time_nonopaque) / txc_commit_attempts);
    }
    if (txp_count >= txp_hco_abort)
        fprintf(stderr, "$ %llu HCO (%llu lock, %llu invalid, %llu aborts)\n",
                out.p(txp_hco), out.p(txp_hco_lock), out.p(txp_hco_invalid), out.p(txp_hco_abort));
    if (txp_count >= txp_hash_collision)
        fprintf(stderr, "$ %llu (%.3f%%) hash collisions, %llu second level\n", out.p(txp_hash_collision),
                100.0 * (double) out.p(txp_hash_collision) / out.p(txp_hash_find),
                out.p(txp_hash_collision2));
    if (txp_count >= txp_total_transbuffer)
        fprintf(stderr, "$ %llu max buffer per txn, %llu total buffer\n",
                out.p(txp_max_transbuffer), out.p(txp_total_transbuffer));
    fprintf(stderr, "$ %llu next commit-tid\n", (unsigned long long) _TID);
}

const char *Transaction::state_name(int state) {
    static const char *names[] = {"in-progress", "opacity-check", "committing", "committing-locked", "aborted",
                                  "committed"};
    if (unsigned(state) < arraysize(names))
        return names[state];
    else
        return "unknown-state";
}


inline void Transaction::serialize_util(unsigned nwriteset) const {
    //pthread_setname_np(pthread_self(), "silo-thread");
    TransItem *it = nullptr;
    size_t w = 0;
    unsigned char *array = NULL;
    size_t ul_len = sizeof(unsigned long long int);

    static thread_local std::shared_ptr<StringAllocator> instance = std::shared_ptr<StringAllocator>(
            new StringAllocator());
    array = instance->getLogOnly(w);

    unsigned long long int cid = 0;  // 8 bytes
    unsigned short int _count = nwriteset;  // 2bytes, the count of K-V pairs
    unsigned short int table_id = 0; // 2 bytes

#if defined(LOG_TO_FILE)
    trans_cnt += 1 ;
#endif

    if (nwriteset == 0) {  // can reduce the log size somehow
        return;
    }

    // 0. commit ID
    cid = tid_unique();

#if defined(LATENCY)
    if (cid % 1000 == 0) {
        mtx2.lock();
        sample_transaction_tracker[cid] = util_cur_usec() ;
        mtx2.unlock();
    }
#endif

    int epoch = 1;
#if !defined(LOG_TO_FILE)
    epoch = callback_();
#endif

    cid = epoch + cid * 1000;
    instance->update_commit_id(cid);

    // 1. copy Commit ID
    memcpy(array + w, (char *) &cid, ul_len);
    w += ul_len;
    // 2. copy the count of K-V pairs
    memcpy(array + w, (char *) &_count, sizeof(unsigned short int));
    w += sizeof(unsigned short int);

    // 3. defer copying the len of K-V pairs
    w += sizeof(unsigned int);
    size_t w_tmp = w;

    unsigned short len_of_K = 0;
    unsigned short len_of_V = 0;

    // to verify and double-check
    size_t _wcount_verify = 0;
    //int flag = 0; // payment = 1, new_order = 2, delivery = 0

    for (unsigned tidx = 0; tidx != tset_size_; ++tidx) {
        table_id = 0x0;

        it = (tidx % tset_chunk ? it + 1 : tset_[tidx / tset_chunk]);
        if (!it->has_write()) {
            continue;
        }
        _wcount_verify += 1;

        // 4. copy the length of key and content of key.
        //    please note, it's not a typo, we have to get the key from transItem.write_value, NOT transItem.key!
        //    check the implementation: MassTrans => trans_write => Sto::new_item(this, val) and add_write
        //    also, due to different implementation purpose, we have to use different ways to retrieve key and value
        std::string kkx = "";
        if (hasInsertOp(it)) {
            kkx = (*it).write_value<std::string>();
        } else {
            kkx = it->extra;
        }
        len_of_K = it->extra.length();
        if (len_of_K == 0) {
            std::cout << "Error while read Key [Slow Exit now]" << std::endl;
            exit(1);
        }

        memcpy(array + w, (char *) &len_of_K, sizeof(unsigned short));
        w += sizeof(unsigned short);

        memcpy(array + w, (char *) kkx.data(), len_of_K);
        w += len_of_K;

        // 5. copy the length of value and content of value
        versioned_str_struct *vvx = (*it).key<versioned_str_struct *>();
        len_of_V = vvx->length();
        memcpy(array + w, (char *) &len_of_V, sizeof(unsigned short));
        w += sizeof(unsigned short);

        memcpy(array + w, (char *) vvx->data(), len_of_V);
        w += len_of_V;

        // 6. copy table id
        table_id = it->sharedObj()->get_identity();
        if (hasDeleteOp(it)) {  // delete flag
            table_id = table_id | (1 << 15); // 1 << ((sizeof(unsigned short)*8)-1) = 1 << 15
        }
        memcpy(array + w, (char *) &table_id, sizeof(unsigned short));
        w += sizeof(unsigned short);

        /*
        if (flag == 0 && table_id >= 10152 && table_id <= 10166) {
            flag = 1;
        } else if (flag == 0 && table_id >= 10122 && table_id <= 10136) {
            flag = 2;
        }*/
        //printf("K: %d, V: %d, has_write: %d, has_insert: %d\n", len_of_K, len_of_V, (*it).has_write(), hasInsertOp(it));
    }
    unsigned int len_of_KV = w - w_tmp;
    memcpy(array + w_tmp - sizeof(unsigned int), (char *) &len_of_KV, sizeof(unsigned int));

    /*
    if (flag == 1) {
        len_payment += (len_of_KV + 14);
    } else if (flag == 2) {
        len_new_order += (len_of_KV + 14);
    } else {
        len_delivery += (len_of_KV + 14);
    } */

    if (_wcount_verify != nwriteset) {
        std::cout << "Error while writing logs [Slow Exit now]" << std::endl;
        exit(1);
    }

#if defined(LOG_TO_FILE)
    // always REGULAR_LOG_SUBMIT allowed in case of log generation to disk
#if defined(REGULAR_LOG_SUBMIT)
        mtx.lock();
        auto ts = static_cast<long int> (std::time(0));
        LogGenRate::increment(ts, w);
        OutputDataSerializer::GetLogger (TThread::id (), LOGGING_CONST::WHOLE_LOG_STRING)->Log ((char *)array, w) ;
        if(w>0){
            std::string _key = "Log-ThreadID:" + std::to_string(TThread::id ());
            ref_output[_key]+= 1;
        }
        mtx.unlock();
        return;
#endif
#endif

    std::string prefix_str = std::to_string(TThread::id());

    /*
     * STO_BATCH_ENABLE_S 0 => REGULAR_LOG_SUBMIT
     * STO_BATCH_ENABLE_S 1 => BULK_SIMPLE_LOG_SUBMIT
     * STO_BATCH_ENABLE_S 2 => BULK_POOL_LOG_SUBMIT
     *
     */
#if defined(REGULAR_LOG_SUBMIT)
    auto startTime = std::chrono::high_resolution_clock::now();
    add_log((char *)array, w, TThread::getPartitionID ());
    auto endTime = std::chrono::high_resolution_clock::now();
    TimerMapper::add_time(prefix_str+"_submit_wait",std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count(),1000.0*1000.0);

#ifdef ALLOW_WAIT_AFTER_EACH_SUBMIT
    startTime = std::chrono::high_resolution_clock::now();
    wait_for_submit(TThread::getPartitionID ());
    endTime = std::chrono::high_resolution_clock::now();
    TimerMapper::add_time(prefix_str+"_wait_for_submit_time_spent",std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count(),1000.0*1000.0);
#endif
#elif defined(BULK_POOL_LOG_SUBMIT)
    instance->update_ptr(w);
    size_t pos = 0;
    unsigned char *queueLog = instance->getLogOnly (pos);
    if(instance->checkPushRequired()) {
#if defined(PAXOS_LIB_ENABLED)
      assert(pos <= MAX_ARRAY_SIZE_IN_BYTES) ;
      if(pos!=0) {
          // 7. latest_commit_id
          memcpy (queueLog + pos, (char *) &cid, sizeof(unsigned long long int));
          pos += sizeof(unsigned long long int);
          instance->update_ptr(pos);

#ifdef ALLOW_PAXOS_INTERCEPT
            paxos_intercept((char *)queueLog, pos, TThread::getPartitionID ());
#else

          #if defined(SINGLE_PAXOS)
          // for single Paxos stream enqueuing
          int outstanding = get_outstanding_logs(TThread::getPartitionID ()) ;
          while (1) {
              if (outstanding >= 200||(outstanding <200 && outstanding>10)) {
                  outstanding = get_outstanding_logs(TThread::getPartitionID ()) ;
                  continue;
              }

              if (outstanding <= 10) {
                  outstanding = get_outstanding_logs(TThread::getPartitionID ()) ;
                  break;
              }
          }
          #endif

        #if !defined(NO_ADD_LOG_TO_NC)
          add_log_to_nc((char *)queueLog, pos, TThread::getPartitionID ());
        #endif
#endif
      }
#endif
      instance->resetMemory();
    }
#endif
}

void Transaction::print(std::ostream &w) const {
    w << "T0x" << (void *) this << " " << state_name(state_) << " [";
    const TransItem *it = nullptr;
    for (unsigned tidx = 0; tidx != tset_size_; ++tidx) {
        it = (tidx % tset_chunk ? it + 1 : tset_[tidx / tset_chunk]);
        if (tidx)
            w << " ";
        it->owner()->print(w, *it);
    }
    w << "]\n";
}

void Transaction::print() const {
    print(std::cerr);
}

void TObject::print(std::ostream &w, const TransItem &item) const {
    w << "{" << typeid(*this).name() << " " << (void *) this << "." << item.key<void *>();
    if (item.has_read())
        w << " R" << item.read_value<void *>();
    if (item.has_write())
        w << " =" << item.write_value<void *>();
    if (item.has_predicate())
        w << " P" << item.predicate_value<void *>();
    w << "}";
}

unsigned long long int TObject::get_identity() const {
    unsigned long long int temp = 10012;
    return temp;
}

std::ostream &operator<<(std::ostream &w, const Transaction &txn) {
    txn.print(w);
    return w;
}

std::ostream &operator<<(std::ostream &w, const TestTransaction &txn) {
    txn.print(w);
    return w;
}

std::ostream &operator<<(std::ostream &w, const TransactionGuard &txn) {
    txn.print(w);
    return w;
}

void Buffer::add(const std::string &array, int pid) {
    if (pid == 0)
        LOG_0 += array;
    else
        LOG_1 += array;
}
