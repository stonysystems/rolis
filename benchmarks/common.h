//
// Created by weihshen on 3/29/21.
//

#ifndef SILO_STO_COMMON_H
#define SILO_STO_COMMON_H

#include "sto/ReplayDB.h"
#include "sto/ThreadPool.h"
#include <iostream>
#include "vector"
#include "abstract_db.h"
#include "deptran/s_main.h"
#include "mbta_wrapper.hh"
#include "bench.h"
#include <thread>
#include <unistd.h>
#include "rpc/server.h"
#include "rpc/client.h"
#include "sto/OutputDataSerializer.h"

uint64_t timeSinceEpochMillisecCommon() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

using namespace std;
// 0: scan, 1: redo, 2: ending; length; data
typedef std::queue<std::tuple<int, int, const char *>> queue_type;
typedef std::vector<queue_type> queue_list;

// global variables shared with dbtest
queue_list qList;
std::vector<std::vector<std::tuple<char *, size_t>>> redo_buffers(32);
bool consume_running = false;
int completed_consume_threads = 0;

// get the global value from the dbtest
std::function<abstract_db *()> callback_getdb_ = nullptr;

void register_getdb_(std::function<abstract_db *()> cb) {
    callback_getdb_ = cb;
    std::cout << "[in common.h]register callback: " << callback_getdb_() << std::endl;
}

static vector<string>
split_ws(const string &s) {
    vector<string> r;
    istringstream iss(s);
    copy(istream_iterator<string>(iss),
         istream_iterator<string>(),
         back_inserter<vector<string>>(r));
    return r;
}

static size_t
parse_memory_spec(const string &s) {
    string x(s);
    size_t mult = 1;
    if (x.back() == 'G') {
        mult = static_cast<size_t>(1) << 30;
        x.pop_back();
    } else if (x.back() == 'M') {
        mult = static_cast<size_t>(1) << 20;
        x.pop_back();
    } else if (x.back() == 'K') {
        mult = static_cast<size_t>(1) << 10;
        x.pop_back();
    }
    return strtoul(x.c_str(), nullptr, 10) * mult;
}

class new_scan_callback : public abstract_ordered_index::scan_callback {
public:
    new_scan_callback() {};

    virtual bool invoke(
            const char *keyp, size_t keylen,
            const std::string &value) {
        values.emplace_back(std::string(keyp, keylen), value);
        // XXX, if scan too much rows and would cause an issue
        if (values.size() >= 2000) {
            return false;
        }
        return true;
    }

    typedef std::pair<std::string, std::string> kv_pair;
    std::vector<kv_pair> values;
};

/*
 * three categories of Paxos log
 *   1. encoded transactions
 *   2. ending signal: empty string
 *   3. no-ops with epoch number, the pattern is "no-ops:4" with the new epoch number 4
 */
bool isNoops(const char *log, int len) {
    // XXXXXFAIL
    if (len > 7) {
        if (log[0] == 'n' && log[1] == 'o' && log[2] == '-' &&
            log[3] == 'o' && log[4] == 'p' && log[5] == 's' && log[6] == ':')
            return true;
    }

    return false;
}

void table_stats(abstract_db *test_db, vector<abstract_ordered_index *> tables) {
    mbta_wrapper *test_db_wrapper = (mbta_wrapper *) test_db;
    TThread::set_id(255);
    actual_directs::thread_init();
    for (auto &item: tables) {
        abstract_ordered_index *table = item;
        mbta_ordered_index *table_mbta = (mbta_ordered_index *) table;
        // ASCII code: 0 ~ 255, range search all potential keys
        char WS = static_cast<char>(0);
        std::string startKey(1, WS);
        char WE = static_cast<char>(255);
        std::string endKey(1, WE);
        new_scan_callback calloc;
        str_arena arena;
        void *buf = NULL;
        void *txnd = test_db_wrapper->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
        table_mbta->scan(txnd, startKey, &endKey, calloc);
        std::cout << "Name of table: " << table_mbta->getName() << ", # of scan db: " << calloc.values.size()
                  << std::endl;
        printf("  e.g., # of K: %lu, # of V: %lu\n", calloc.values[0].first.length(), calloc.values[0].second.length());
        test_db_wrapper->commit_txn_no_paxos(txnd);
    }
}

void dbScan(abstract_db *test_db) {
    mbta_wrapper *test_db_wrapper = (mbta_wrapper *) test_db;
    vector<abstract_ordered_index *> ret;
    for (auto &item: test_db_wrapper->table_instances) {
        abstract_ordered_index *table = item.second;
        ret.emplace_back(table);
    }
    table_stats(test_db, ret);
}

void start_SiLo_workers_tpcc_dummpy(abstract_db *db, int threads_nums, bool skip_load = false) {
    std::cout << "dummpy function is invoked: " << threads_nums << std::endl;
}

void start_Silo_workers_micro(abstract_db *db, int threads_nums, int argc, string bench_opts) {
    string bench_type = "micro";
    vector<string> bench_toks = split_ws(bench_opts);
    int micro_argc = 1 + bench_toks.size();
    char *micro_argv[argc];
    micro_argv[0] = (char *) bench_type.c_str();
    for (size_t i = 1; i <= bench_toks.size(); i++)
        micro_argv[i] = (char *) bench_toks[i - 1].c_str();
    bench_runner *R = rsimple_do_test(db, micro_argc, micro_argv);

#if !defined(LOG_TO_FILE) && defined(PAXOS_LIB_ENABLED)
    // send the ending signal
    std::string endLogInd = "";
    for (int i = 0; i < threads_nums; i++)
        add_log_to_nc((char *) endLogInd.c_str(), 0, i);
#endif

    // wait for all acknowledges
#if defined(ALLOW_WAIT_AT_PAXOS_END) && defined(PAXOS_LIB_ENABLED)
    vector<std::thread> wait_threads;
      for(int i = 0; i < nthreads; i++){
          wait_threads.push_back(std::thread([i](){
           std::cout << "Starting wait for " << i << std::endl;
           wait_for_submit(i);
          }));
      }
      for (auto& th : wait_threads) {
          th.join();
      }
#endif
    R->print_stats();
}


void start_SiLo_workers_tpcc(abstract_db *db, int threads_nums, bool skip_load = false) {
    std::string bench_type = "tpcc";
    std::string bench_opts = "--cpu-gap 1 --num-cpus 80 --f_mode=0";
    if (skip_load) {
        bench_opts = "--cpu-gap 1 --num-cpus 80 --f_mode=1";
    }

    vector<string> bench_toks = split_ws(bench_opts);
    int argc_bench = 1 + bench_toks.size();
    char *argv_bench[argc_bench];
    argv_bench[0] = (char *) bench_type.c_str();
    for (size_t i = 1; i <= bench_toks.size(); i++) {
        argv_bench[i] = (char *) bench_toks[i - 1].c_str();
    }
    bench_runner *R = tpcc_do_test_run(db, argc_bench, argv_bench);
#if !defined(LOG_TO_FILE) && defined(PAXOS_LIB_ENABLED)
    std::string endLogInd="";
    for(int i=0; i<threads_nums; i++)
        add_log_to_nc((char *)endLogInd.c_str(), 0, i);
#endif

#if defined(ALLOW_WAIT_AT_PAXOS_END) && defined(PAXOS_LIB_ENABLED)
    vector<std::thread> wait_threads;
      for(int i = 0; i < nthreads; i++){
          wait_threads.push_back(std::thread([i](){
           std::cout << "Starting wait for " << i << std::endl;
           wait_for_submit(i);
          }));
      }
      for (auto& th : wait_threads) {
          th.join();
      }
#endif

    R->print_stats();

    mbta_wrapper *test_db_wrapper = (mbta_wrapper *) db;
    vector<abstract_ordered_index *> ret;
    for (auto &item: R->get_open_tables()) {
        abstract_ordered_index *table = item.second;
        ret.emplace_back(table);
    }
    // table_stats(db, ret) ;
}

// init db
abstract_db *ThreadDBWrapperMbta::replay_thread_wrapper_db = new mbta_wrapper;

void mimicMainPaxosRun() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    {
        std::lock_guard<std::mutex> lk((sync_util::sync_logger::m));
        sync_util::sync_logger::toLeader = true;
        std::cout << "notify a new leader is elected - mimic!\n";
    }
    sync_util::sync_logger::cv.notify_one();
}

void mimicMainPaxos() {
    thread mimic_thread(&mimicMainPaxosRun);
    pthread_setname_np(mimic_thread.native_handle(), "mimicMainPaxos");
    mimic_thread.detach();  // thread detach
}

void modeMonitorRun(abstract_db *db, int thread_nums) {
    // Wait until mainPaxos sends data
    std::unique_lock<std::mutex> lk((sync_util::sync_logger::m));
    sync_util::sync_logger::cv.wait(lk, [] { return sync_util::sync_logger::toLeader; });

    if (!sync_util::sync_logger::running) {
        return;
    }

    // after the wait, we own the lock
    start_SiLo_workers_tpcc(db, thread_nums, true);

    lk.unlock();
    sync_util::sync_logger::cv.notify_one();
}

void modeMonitor(abstract_db *db, int thread_nums) {
    thread mimic_thread(&modeMonitorRun, db, thread_nums);
    pthread_setname_np(mimic_thread.native_handle(), "modeMonitor");
    mimic_thread.detach();  // thread detach
}


// ---------------------------------------- thread on the older leader
class new_scan_callback_bench : public abstract_ordered_index::scan_callback {
public:
    new_scan_callback_bench() {
    }
    bool invoke(
            const char *keyp, size_t keylen,
            const string &value) override
    {
        if (values.empty() && skip_firstrow) { skip_firstrow=false; return true; };
        values.emplace_back(std::string(keyp, keylen), value);
        if (values.size() == limit) {
            return false;
        }
        return true;
    }

    void set_limit(size_t llimit=10000) {limit=llimit;};
    void set_skip_firstrow(bool sskip_firstrow=false) {skip_firstrow=sskip_firstrow;};

    typedef std::pair<std::string, std::string> kv_pair;
    std::vector<kv_pair> values;
    bool skip_firstrow{false};
    size_t limit{INT_MAX};
};


void consumeLogs(int par_id) {
    std::cout << "start to consume index: " << par_id << std::endl;
    int tag = 0;
    while (consume_running || qList[par_id].size() > 0) {  // while-1
        while (qList[par_id].size() > 0) {  // while-2
            std::tuple<int, int, const char *> it = qList[par_id].front();
            qList[par_id].pop();
            if (std::get<0>(it) == 0) { // scanned logs
                treplay_in_same_thread_scanned_logs(par_id,
                                                    (char*)std::get<2>(it),
                                                    std::get<1>(it),
                                                    callback_getdb_()) ;
            } else if (std::get<0>(it) == 1) { // redo logs
                std::cout << "deal with a redo logs\n";
                treplay_in_same_thread_opt_mbta_v2(par_id,
                                                   (char*)std::get<2>(it),
                                                   std::get<1>(it),
                                                   callback_getdb_());
            } else {
                std::cout << "consume: " << par_id << " has completed~\n";
                break;
            }
        }  // end of while-2
        usleep(1000 * 10); // 10 ms
        if (tag++ % 100 == 0) {
            std::cout << "qList size: " << qList[par_id].size() << ", par_id: " << par_id << std::endl;
        }
    }
    completed_consume_threads++;
}

// start {thread_nums} threads to replay logs in each queue (qList[i])
void initAddFollowerReplay(int thread_nums) {
    for (int i = 0; i < thread_nums; ++i) {
        thread consume_thread(&consumeLogs, i);
        pthread_setname_np(consume_thread.native_handle(), "consume_thread");
        consume_thread.detach();
    }
}

int onScannedLogs(string logs, int index) {
    //std::cout << "receive a onScannedLogs (size): " << logs.size() << std::endl;
    //usleep(1 * 1000 * 1000);
    char *dest = (char *)malloc(logs.size()) ;
    memcpy(dest, logs.c_str(), logs.size()) ;
    qList[index % nthreads].push(std::make_tuple(0, logs.size(), dest));
    return 0;
}

int onRedoLogs(string logs, int index) {
    std::cout << "receive a onRedoLogs: " << logs << std::endl;
    char *dest = (char *)malloc(logs.size()) ;
    memcpy(dest, logs.c_str(), logs.size()) ;
    qList[index % nthreads].push(std::make_tuple(1, logs.size(), dest));
    return 0;
}

void afterLogDONE() {
    // then send "all_DONE" to the sync_source
    rpc::client client(sync_util::sync_logger::rpc_sync_host, 8080 + sync_util::sync_logger::rpc_sync_port);
    std::cout << "send onAllDone...\n";
    auto result = client.call("onAllDone").as<int>();
}

int onLogDone() {
    std::cout << "receive a onLogDone" << std::endl;
    thread start_thread(&afterLogDONE);
    pthread_setname_np(start_thread.native_handle(), "afterLogDONE");
    start_thread.detach();
    return 0;
}

void older_leader_server() {
    // Creating a server that listens on port 8080
    rpc::server srv(8080);

    srv.bind("onScannedLogs", &onScannedLogs);
    srv.bind("onRedoLogs", &onRedoLogs);
    srv.bind("onLogDone", &onLogDone);

    srv.run();
}

// ---------------------------------------- thread on the follower
void follower_client() {
    // 0. connect to the old leader, hardcoded (10.1.0.7)
    rpc::client client("127.0.0.1", 8080);

    // 1. start to send scanned logs, format: table_id|key|value|table_id|key|value
    string msg = "";

    mbta_wrapper *test_db_wrapper = (mbta_wrapper *) callback_getdb_();
    vector<abstract_ordered_index *> ret;
    int idx=0;
    for (auto &item: test_db_wrapper->table_instances) {
        abstract_ordered_index *table = item.second;
        std::string table_id = std::to_string(item.first);
        std::cout << "start to scan table_id: " << table_id << std::endl;
        str_arena arena;
        void *buf = NULL;
        char WS = static_cast<char>(0);
        std::string startKey(1, WS);
        char WE = static_cast<char>(255);
        std::string endKey(1, WE);
        std::string end_iter;
        bool startBulk = true;
        size_t bulkSize = 0;
        int tag=0;
        do {
            ++tag;
            void *txn0 = test_db_wrapper->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
            new_scan_callback_bench calloc;
            calloc.set_limit(100);
            calloc.set_skip_firstrow(!startBulk);
            end_iter = table->scan_iter(txn0, startKey, &endKey, calloc);
            startKey = end_iter;
            startBulk = false;
            bulkSize = calloc.values.size();
            msg = "";
            test_db_wrapper->commit_txn_no_paxos(txn0);

            for (int i = 0; i < bulkSize; i++) {
                msg += table_id + "|" \
                        + std::to_string(calloc.values[i].first.size()) + "|" \
                        + std::to_string(calloc.values[i].second.size()) + "|" \
                        + calloc.values[i].first + calloc.values[i].second;
            }
            //std::cout << "send a onScannedLogs (size): " << msg.size() << ", tag: " << tag << std::endl;
            if (msg.size() > 0) {
                //std::cout << "send scanned logs: " << idx << "\n";
                client.call("onScannedLogs", msg, idx).as<int>();
                //auto serializer = OutputDataSerializer::GetLogger (idx, "./logs/");
                //serializer->Log(msg.c_str(), msg.length());
                idx++;
                //serializer->close_all();
            }
        } while (bulkSize);
        std::cout << "finish to scan table_id: " << table_id << std::endl;
    }

    // 2. stop recording redo-logs
    std::cout << "stop to consume running...." ;
    consume_running = false;

    // 3. start to send redo logs
    // TODO, use multiple sockets to send redo logs
    for (int tid = 0; tid < 32; tid++) {
        for (int j = 0; j < redo_buffers[tid].size(); j++) {
            auto it = redo_buffers[tid][j];
            std::string data = std::string((char *) std::get<0>(it), std::get<1>(it));
            client.call("onRedoLogs", data, tid).as<int>();
        }
    }
     /*msg = "reXXXXXXXXXXXXXXXXXXXXX";
     usleep(1 * 1000 * 1000);
     std::cout << "send the redo logs: " << msg << std::endl;
     client.call("onRedoLogs", msg, 0).as<int>();*/

    // 4. send the onLogDone msg
    std::cout << "send the onLogDone" << std::endl;
    client.call("onLogDone").as<int>();
}

int onGetRole() { // 0: leader, 1: follower
    std::cout << "receive onGetRole...........................\n";
    return sync_util::sync_logger::toLeader ? 0 : 1;
}

int onSyncSource() {
    std::cout << "receive onSyncSource\n";

    thread start_follower_client(&follower_client);
    pthread_setname_np(start_follower_client.native_handle(), "start_follower_client");
    start_follower_client.detach();
    return 0;
}

int onAllDone() {
    std::cout << "receive onAllDone" << std::endl;
    // TODO, kill all sub-threads
    return 0;
}

void follower_server(abstract_db *db, int rpc_ext) {
    std::cout << "XXXXXXXXXXXX: start a follower rpc server: " << (8080 + rpc_ext) << std::endl;
    rpc::server srv(8080 + rpc_ext);
    srv.bind("onGetRole", &onGetRole);
    srv.bind("onSyncSource", &onSyncSource);
    srv.bind("onAllDone", &onAllDone);
    srv.run();
}

int get_server_rpc_port(std::string proc_name) {
    std::cout << "XXXXXXXXXXXXXXX: " << len_payment << std::endl;
    int server_rpc_ext = 0;
    if (proc_name.compare("p1") == 0) {
        server_rpc_ext = 1;
    } else if (proc_name.compare("p2") == 0) {
        server_rpc_ext = 2;
    } else {
        std::cout << "can't support " << proc_name << std::endl;
        exit(1);
    }
    // leader must listen on port 8080
    return server_rpc_ext;
}

#endif //SILO_STO_COMMON_H

