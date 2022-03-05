#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <string>
#include <set>
#include <tuple>
#include <vector>
#include <list>

#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "../allocator.h"
#include "../stats_server.h"
#include "bench.h"
#include "ndb_wrapper.h"
#include "ndb_wrapper_impl.h"
#include "kvdb_wrapper.h"
#include "kvdb_wrapper_impl.h"
#include "sto/sync_util.hh"
#include "sto/Transaction.hh"
#include "mbta_wrapper.hh"
#include "gperftools/profiler.h"
#include "gperftools/heap-profiler.h"
#include "deptran/s_main.h"
#include "common.h"

using namespace std;
using namespace util;
using namespace sync_util;

//#define DBTEST_PROFILER_ENABLED
//#define USE_JEMALLOC


int
main(int argc, char **argv) {
    // ./third-party/paxos/scripts/pprof --pdf ./out-perf.masstree/benchmarks/dbtest dbtest.prof > dbtest.pdf
#if defined DBTEST_PROFILER_ENABLED
  #ifdef USE_JEMALLOC
    ProfilerStart("dbtest.prof");
  #elif defined USE_TCMALLOC
    //HeapProfilerStart("dbtest.prof");
  #endif
#endif
    vector<string> paxos_config_file{};
    string bench_type = "tpcc";
    int kSTOBatchSize = 1000;
    int kPaxosBatchSize = 50000;

    string db_type = "mbta";
    char *curdir = get_current_dir_name();
    string basedir = curdir;
    string bench_opts;
    size_t numa_memory = 0;
    free(curdir);
    int saw_run_spec = 0;
    int nofsync = 0;
    int do_compress = 0;
    int fake_writes = 0;
    int sender = 0; // sender or receiver
    int disable_gc = 0;
    int leader_config = 0;
    int multi_process = 0;
    int disable_snapshots = 0;
    string paxos_proc_name = "localhost";
    vector<string> logfiles;
    vector<vector<unsigned>> assignments;
    string stats_server_sockfile;
    // track latency: advanceGTracker, latestG
    std::vector<std::pair<uint64_t, uint64_t>> advanceGTracker;  // G => updated time
    uint64_t latestG = 0;
    // track ready_queue latency
    std::vector<int> readyQueueTracker;  // the logs to be replayed, 1 log == 1000 transactions




    while (1) {
        static struct option long_options[] =
                {
                        {"verbose",                      no_argument,       &verbose,                     1},
                        {"parallel-loading",             no_argument,       &enable_parallel_loading,     1},
                        {"pin-cpus",                     no_argument,       &pin_cpus,                    1},
                        {"slow-exit",                    no_argument,       &slow_exit,                   1},
                        {"retry-aborted-transactions",   no_argument,       &retry_aborted_transaction,   1},
                        {"backoff-aborted-transactions", no_argument,       &backoff_aborted_transaction, 1},
                        {"paxos-leader-config",          no_argument,       &leader_config,               1},
                        {"multi-process",                no_argument,       &multi_process,               1},
                        {"log-nofsync",                  no_argument,       &nofsync,                     1},
                        {"log-compress",                 no_argument,       &do_compress,                 1},
                        {"log-fake-writes",              no_argument,       &fake_writes,                 1},
                        {"disable-gc",                   no_argument,       &disable_gc,                  1},
                        {"disable-snapshots",            no_argument,       &disable_snapshots,           1},
                        {"no-reset-counters",            no_argument,       &no_reset_counters,           1},
                        {"use-hashtable",                no_argument,       &use_hashtable,               1},
                        {"p-batch-size",                 optional_argument, 0,                            'A'},
                        {"assignment",                   required_argument, 0,                            'a'},
                        {"basedir",                      required_argument, 0,                            'B'},
                        {"bench",                        required_argument, 0,                            'b'},
                        {"db-type",                      required_argument, 0,                            'd'},
                        {"logfile",                      required_argument, 0,                            'l'},
                        {"numa-memory",                  required_argument, 0,                            'm'}, // implies --pin-cpus
                        {"ops-per-worker",               required_argument, 0,                            'n'},
                        {"bench-opts",                   required_argument, 0,                            'o'},
                        {"paxos-config",                 required_argument, 0,                            'F'},
                        {"txn-flags",                    required_argument, 0,                            'f'},
                        {"paxos-proc-name",              required_argument, 0,                            'P'},
                        {"runtime",                      required_argument, 0,                            'r'},
                        {"sto-batch-size",               optional_argument, 0,                            'S'},
                        {"scale-factor",                 required_argument, 0,                            's'},
                        {"num-threads",                  required_argument, 0,                            't'},
                        {"stats-server-sockfile",        required_argument, 0,                            'x'},
                        {"sender",                       required_argument, 0,                            'z'},
                        {0, 0,                                              0,                            0}
                };
        int option_index = 0;
        int c = getopt_long(argc, argv, "b:s:t:d:z:B:f:r:n:o:m:l:a:e:x:F:A:S:P:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                abort();
                break;

            case 'b':
                bench_type = optarg;
                break;

            case 'P':
                paxos_proc_name = string(optarg);
                break;

            case 'A':
                kPaxosBatchSize = strtoul(optarg, NULL, 10);
                break;

            case 'S':
                kSTOBatchSize = strtoul(optarg, NULL, 10);
                break;

            case 's':
                scale_factor = strtod(optarg, NULL);
                ALWAYS_ASSERT(scale_factor > 0.0);
                break;

            case 't':
                nthreads = strtoul(optarg, NULL, 10);
                ALWAYS_ASSERT(nthreads > 0);
                break;

            case 'd':
                db_type = optarg;
                break;

            case 'B':
                basedir = optarg;
                break;

            case 'f':
                txn_flags = strtoul(optarg, NULL, 10);
                break;

            case 'r':
                ALWAYS_ASSERT(!saw_run_spec);
                saw_run_spec = 1;
                runtime = strtoul(optarg, NULL, 10);
                ALWAYS_ASSERT(runtime > 0);
                run_mode = RUNMODE_TIME;
                break;

            case 'z':
                sender = strtoul(optarg, NULL, 10);
                break;

            case 'n':
                ALWAYS_ASSERT(!saw_run_spec);
                saw_run_spec = 1;
                ops_per_worker = strtoul(optarg, NULL, 10);
                ALWAYS_ASSERT(ops_per_worker > 0);
                run_mode = RUNMODE_OPS;

            case 'o':
                bench_opts = optarg;
                break;

            case 'm': {
                pin_cpus = 1;
                const size_t m = parse_memory_spec(optarg);
                ALWAYS_ASSERT(m > 0);
                numa_memory = m;
            }
                break;

            case 'l':
                logfiles.emplace_back(optarg);
                break;

            case 'a':
                assignments.emplace_back(
                        ParseCSVString<unsigned, RangeAwareParser<unsigned>>(optarg));
                break;

            case 'x':
                stats_server_sockfile = optarg;
                break;

            case 'F':
                paxos_config_file.push_back(optarg);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                exit(1);

            default:
                abort();
        }
    }

    abstract_db *db = NULL;
    bench_runner *(*test_fn_with_runner)(abstract_db *, int argc, char **argv) = NULL;

    if (leader_config || !multi_process) {
        /*
        #if 0
        if (bench_type == "ycsb")
            test_fn = ycsb_do_test;
        else if (bench_type == "tpccSimple")
            test_fn = tpcc_simple_do_test;
        else if (bench_type == "tpcc")
            test_fn = tpcc_do_test;
        else if (bench_type == "queue")
            test_fn = queue_do_test;
        else if (bench_type == "encstress")
            test_fn = encstress_do_test;
        else if (bench_type == "bid")
            test_fn = bid_do_test;
        else
            ALWAYS_ASSERT(false);
        #endif  */
        if (bench_type == "tpcc")
            test_fn_with_runner = tpcc_do_test_run;
        else if (bench_type == "micro")
            test_fn_with_runner = rsimple_do_test;
        else
            ALWAYS_ASSERT(false);

    }

    if (do_compress && logfiles.empty()) {
        cerr << "[ERROR] --log-compress specified without logging enabled" << endl;
        return 1;
    }

    if (fake_writes && logfiles.empty()) {
        cerr << "[ERROR] --log-fake-writes specified without logging enabled" << endl;
        return 1;
    }

    if (nofsync && logfiles.empty()) {
        cerr << "[ERROR] --log-nofsync specified without logging enabled" << endl;
        return 1;
    }

    if (fake_writes && nofsync) {
        cerr << "[WARNING] --log-nofsync has no effect with --log-fake-writes enabled" << endl;
    }

#ifndef ENABLE_EVENT_COUNTERS
    if (!stats_server_sockfile.empty()) {
        cerr << "[WARNING] --stats-server-sockfile with no event counters enabled is useless" << endl;
    }
#endif
    // start a detached thread to advance G_ = min(cg_) and cg_ (latest_commit_id of paxos group)
    sync_util::sync_logger::Init(nthreads);

    if (true || leader_config || !multi_process) {
        // initialize the numa allocator
        if (numa_memory > 0) {
            const size_t maxpercpu = util::iceil(
                    numa_memory / nthreads, ::allocator::GetHugepageSize());
            numa_memory = maxpercpu * nthreads;
            ::allocator::Initialize(nthreads, maxpercpu);
        }

        const set<string> can_persist({"ndb-proto2"});
        if (!logfiles.empty() && !can_persist.count(db_type)) {
            cerr << "[ERROR] benchmark " << db_type
                 << " does not have persistence implemented" << endl;
            return 1;
        }

#ifdef PROTO2_CAN_DISABLE_GC
        const set<string> has_gc({"ndb-proto1", "ndb-proto2"});
        if (disable_gc && !has_gc.count(db_type)) {
            cerr << "[ERROR] benchmark " << db_type
                 << " does not have gc to disable" << endl;
            return 1;
        }
#else
        if (disable_gc) {
            cerr << "[ERROR] macro PROTO2_CAN_DISABLE_GC was not set, cannot disable gc" << endl;
            return 1;
        }
#endif

#ifdef PROTO2_CAN_DISABLE_SNAPSHOTS
        const set<string> has_snapshots({"ndb-proto2"});
        if (disable_snapshots && !has_snapshots.count(db_type)) {
            cerr << "[ERROR] benchmark " << db_type
                 << " does not have snapshots to disable" << endl;
            return 1;
        }
#else
        if (disable_snapshots) {
            cerr << "[ERROR] macro PROTO2_CAN_DISABLE_SNAPSHOTS was not set, cannot disable snapshots" << endl;
            return 1;
        }
#endif

        if (db_type == "ndb-proto1") {
            // XXX: hacky simulation of proto1
            /*
            db = new ndb_wrapper<transaction_proto2>(
                logfiles, assignments, !nofsync, do_compress, fake_writes);
            transaction_proto2_static::set_hack_status(true);
            ALWAYS_ASSERT(transaction_proto2_static::get_hack_status());
            #ifdef PROTO2_CAN_DISABLE_GC
            if (!disable_gc)
              transaction_proto2_static::InitGC();
            #endif */
            ALWAYS_ASSERT(false);
        } else if (db_type == "ndb-proto2") {
            /*
            db = new ndb_wrapper<transaction_proto2>(
                logfiles, assignments, !nofsync, do_compress, fake_writes);
            ALWAYS_ASSERT(!transaction_proto2_static::get_hack_status());
            #ifdef PROTO2_CAN_DISABLE_GC
            if (!disable_gc)
              transaction_proto2_static::InitGC();
            #endif
            #ifdef PROTO2_CAN_DISABLE_SNAPSHOTS
            if (disable_snapshots)
              transaction_proto2_static::DisableSnapshots();
            #endif */
            ALWAYS_ASSERT(false);
        } else if (db_type == "kvdb") {
            /*
            db = new kvdb_wrapper<true>; */
            ALWAYS_ASSERT(false);
        } else if (db_type == "kvdb-st") {
            /*
            db = new kvdb_wrapper<false>; */
            ALWAYS_ASSERT(false);
        } else if (db_type == "mbta") {
            db = new mbta_wrapper;
        } else
            ALWAYS_ASSERT(false);
    }

#ifdef DEBUG
    cerr << "WARNING: benchmark built in DEBUG mode!!!" << endl;
#endif

#ifdef CHECK_INVARIANTS
    cerr << "WARNING: invariant checking is enabled - should disable for benchmark" << endl;
#ifdef PARANOID_CHECKING
    cerr << "  *** Paranoid checking is enabled ***" << endl;
#endif
#endif
    std::string SUFFIX_STRING = "GenLogThd" + std::to_string(nthreads) + ".Time." + std::to_string(runtime) + "/";
    LOGGING_CONST::setlog(nthreads, runtime);

    if (verbose) {
        const unsigned long ncpus = coreid::num_cpus_online();
        cerr << "Database Benchmark:" << endl;
        cerr << "  pid: " << getpid() << endl;
        cerr << "settings:" << endl;
        cerr << "  par-loading : " << enable_parallel_loading << endl;
        cerr << "  pin-cpus    : " << pin_cpus << endl;
        cerr << "  slow-exit   : " << slow_exit << endl;
        cerr << "  retry-txns  : " << retry_aborted_transaction << endl;
        cerr << "  backoff-txns: " << backoff_aborted_transaction << endl;
        cerr << "  bench       : " << bench_type << endl;
        cerr << "  scale       : " << scale_factor << endl;
        cerr << "  num-cpus    : " << ncpus << endl;
        cerr << "  num-threads : " << nthreads << endl;
        cerr << "  db-type     : " << db_type << endl;
        cerr << "  basedir     : " << basedir << endl;
        cerr << "  txn-flags   : " << hexify(txn_flags) << endl;
        cerr << "  Output log  : " << LOGGING_CONST::WHOLE_LOG_STRING << endl;
        if (run_mode == RUNMODE_TIME)
            cerr << "  runtime     : " << runtime << endl;
        else
            cerr << "  ops/worker  : " << ops_per_worker << endl;
#ifdef USE_VARINT_ENCODING
        cerr << "  var-encode  : yes" << endl;
#else
        cerr << "  var-encode  : no"                            << endl;
#endif

#ifdef USE_JEMALLOC
        cerr << "  allocator   : jemalloc"                      << endl;
#elif defined USE_TCMALLOC
        cerr << "  allocator   : tcmalloc"                      << endl;
#elif defined USE_FLOW
        cerr << "  allocator   : flow"                          << endl;
#else
        cerr << "  allocator   : libc" << endl;
#endif
        if (numa_memory > 0) {
            cerr << "  numa-memory : " << numa_memory << endl;
        } else {
            cerr << "  numa-memory : disabled" << endl;
        }
        cerr << "  logfiles : " << logfiles << endl;
        cerr << "  assignments : " << assignments << endl;
        cerr << "  disable-gc : " << disable_gc << endl;
        cerr << "  disable-snapshots : " << disable_snapshots << endl;
        cerr << "  stats-server-sockfile: " << stats_server_sockfile << endl;

        cerr << "system properties:" << endl;
        cerr << "  btree_internal_node_size: " << concurrent_btree::InternalNodeSize() << endl;
        cerr << "  btree_leaf_node_size    : " << concurrent_btree::LeafNodeSize() << endl;

#ifdef TUPLE_PREFETCH
        cerr << "  tuple_prefetch          : yes" << endl;
#else
        cerr << "  tuple_prefetch          : no" << endl;
#endif

#ifdef BTREE_NODE_PREFETCH
        cerr << "  btree_node_prefetch     : yes" << endl;
#else
        cerr << "  btree_node_prefetch     : no" << endl;
        cerr << "  leader_config : " << leader_config << " & multi-process : " << multi_process <<  << endl;
#endif

    }

    if ((leader_config || !multi_process) && !stats_server_sockfile.empty()) {
        stats_server *srvr = new stats_server(stats_server_sockfile);
        thread(&stats_server::serve_forever, srvr).detach();
    }
    int argc_paxos = 18;
    int k = 0;
    char *argv_paxos[argc_paxos];
    if (paxos_config_file.size() < 2) {
        cerr << "no enough paxos config files" << endl;
        return 1;
    }
    argv_paxos[0] = (char *) "third-party/paxos/build/microbench";
    argv_paxos[1] = (char *) "-b";
    argv_paxos[2] = (char *) "-d";
    argv_paxos[3] = (char *) "60";
    argv_paxos[4] = (char *) "-f";
    argv_paxos[5] = (char *) paxos_config_file[k++].c_str();
    argv_paxos[6] = (char *) "-f";
    argv_paxos[7] = (char *) paxos_config_file[k++].c_str();
    argv_paxos[8] = (char *) "-t";
    argv_paxos[9] = (char *) "30";
    argv_paxos[10] = (char *) "-T";
    argv_paxos[11] = (char *) "100000";
    argv_paxos[12] = (char *) "-n";
    argv_paxos[13] = (char *) "32";
    argv_paxos[14] = (char *) "-P";
    argv_paxos[15] = (char *) paxos_proc_name.c_str();
    argv_paxos[16] = (char *) "-A";
    argv_paxos[17] = new char[20];
    memset(argv_paxos[17], '\0', 20);
    sprintf(argv_paxos[17], "%d", kPaxosBatchSize);

    std::cout << "paxos batch=" << argv_paxos[17] << " :::: " << kPaxosBatchSize << std::endl;

    if (leader_config) {
#ifndef PAXOS_LEADER_HERE
#define PAXOS_LEADER_HERE 1
#endif
    }

#ifdef PAXOS_LEADER_HERE
    std::map<std::string, size_t> _leader_callbacks = {};
    std::map<std::string, size_t> _follower_callbacks = {};

#endif
    static std::atomic<int> count(0);
    static std::atomic<bool> end_recv(false);



#if defined(PAXOS_LIB_ENABLED)
    std::cout << "Paxos enabled" << std::endl;
    std::vector<std::string> ret = setup(argc_paxos, argv_paxos);
    if (ret.empty()) {
      return -1;
    }

#if ALLOW_FOLLOWER_REPLAY
    std::cout << "SiLO-Paxos follower replay enabled" << std::endl;
    //TSharedThreadPool tpool (nthreads);
    //tpool.initPool (nthreads);

    TSharedThreadPoolMbta tpool_mbta (nthreads);
    tpool_mbta.initPool (nthreads);

    // instead sending logs to Paxos, replay logs immediately in SiLO thread
#ifdef ALLOW_PAXOS_INTERCEPT
        paxos_intercept = [&](const char* log, int len, int par_id) {
            if (par_id != 0) { // XXX, skip for par_id: 0 temporarily because the thread with id=0 created in load phase has been destroyed in worker phase
                //auto num = treplay_in_same_thread_opt(par_id, (char *) log, len, std::ref(tpool.getDBWrapper(par_id)->getDB ()), table_logger::table_set);
            }
        };
#endif

#else
    std::cout << "SiLO-Paxos follower replay disabled" << std::endl;
#endif

  register_sync_util([&]() {
    #if defined(FAIL_OVER)
        return get_epoch();
    #else
        return 1;
    #endif
  });

  register_leader_election_callback([&]() {
      std::lock_guard<std::mutex> lk((sync_util::sync_logger::m));
      sync_util::sync_logger::toLeader = true ;
      std::cout << "notify a new leader is elected!\n" ;
      sync_util::sync_logger::cv.notify_one();
  });

  for (int i = 0; i < nthreads; i++) {
    register_for_follower_par_id_return([&,i](const char*& log, int len, int par_id, std::queue<std::tuple<unsigned long long int, int, int, const char *>> & un_replay_logs_) {
      #if defined(SINGLE_PAXOS) || defined(DISABLE_REPLAY)
      return 4;
      #else 
      abstract_db * db = tpool_mbta.getDBWrapper(par_id)->getDB () ;

      // status: 1 => init, 2 => ending of paxos group, 3 => can't pass the safety check, 4 => complete replay, 5 => noops
      int status = 1 ;
      unsigned long long int latest_commit_id = 0 ;
      bool stats = false ;
      bool local_end_recv = false ;
      bool noops = false ;

      if(len == 0){
        std::cout << "par_id " << par_id << " is ended..." << std::endl;
        end_recv = true;
        count ++;
        local_end_recv = true ;
        status = 2; // ending of one paxos group
        uint64_t min_so_far = numeric_limits<uint64_t>::max();
        sync_util::sync_logger::cg_[par_id].store(min_so_far, memory_order_release) ;   // latest_commit_id for each partition should be non-descending
      }
#if ALLOW_FOLLOWER_REPLAY
      if (!local_end_recv) {
         // XXX, no more than 1 failure happen during this switch period
         if (isNoops(log, len)) {
            noops = true ;
            status = 5 ;
         }
         auto startTime = std::chrono::high_resolution_clock::now ();
         // readyQueueTracker.push_back( un_replay_logs_.size() ) ;
#ifdef OPTIMIZED_VERSION
          if (noops) {
          printf("1: received a no-ops, par_id: %d\n", par_id);
              // need to wait until all followers can get thie noops
              sync_util::sync_logger::noops_cnt ++ ;
              while (1) {
                  // check if all threads receive noops
                  if (sync_util::sync_logger::noops_cnt.load(memory_order_acquire) % nthreads == 0) {
                  printf("1-1: received a no-ops, par_id: %d, cnt: %d\n", par_id, sync_util::sync_logger::noops_cnt.load(memory_order_acquire));
                      break ;
                  } else {
                      std::this_thread::sleep_for(std::chrono::milliseconds(10));
                  }
              }
          } else {
              latest_commit_id = get_latest_commit_id ((char *) log, len) ;
              latest_commit_id = latest_commit_id / 1000 ;
              int epoch = latest_commit_id % 1000 ;
              sync_util::sync_logger::cg_[par_id].store(latest_commit_id, memory_order_release) ;
              auto g = sync_util::sync_logger::retrieveG() ;
              if (latest_commit_id > g) {
                  status = 3 ; // can't pass the safety check
              } else {
                  //auto nums = treplay_in_same_thread_opt(par_id, (char *) log, len, std::ref(tpool.getDBWrapper(par_id)->getDB ()), table_logger::table_set);
                  //auto nums = treplay_in_same_thread_opt_mbta(par_id, (char *) log, len, db, table_logger::table_set);
                  if (consume_running) {
                      redo_buffers[par_id].push_back(std::tuple((char *) log, len));
                  }
                  auto nums = treplay_in_same_thread_opt_mbta_v2(par_id, (char *) log, len, db);
                  status = 4 ; // complete replay
              }
          }

          uint64_t g = 0 ;
          if (noops) {
              g = sync_util::sync_logger::computeG() ;
          } else {
              g = sync_util::sync_logger::retrieveG(false) ;
          }
          // check if we can replay in ready_queue
          while ( un_replay_logs_.size() > 0 ) {
              auto it = un_replay_logs_.front() ;
              if (std::get<0>(it) <= g) {
                  //auto nums = treplay_in_same_thread_opt(par_id, (char *) std::get<3>(it), std::get<2>(it), std::ref(tpool.getDBWrapper(par_id)->getDB ()), table_logger::table_set);
                  //auto nums = treplay_in_same_thread_opt_mbta(par_id, (char *) std::get<3>(it), std::get<2>(it), db, table_logger::table_set);
                  if (consume_running) {
                      redo_buffers[par_id].push_back(std::tuple((char *) std::get<3>(it), std::get<2>(it)));
                  }
                  auto nums = treplay_in_same_thread_opt_mbta_v2(par_id, (char *) std::get<3>(it), std::get<2>(it), db);
                  un_replay_logs_.pop() ;
                  free((char*)std::get<3>(it));
              } else {
                  if (noops) {
                      un_replay_logs_.pop() ;
                      free((char*)std::get<3>(it));
                  } else {
                      break ;
                  }
              }
          }

          if (noops) {
              // need to wait until upon all followers skipping holes
              sync_util::sync_logger::noops_cnt_hole ++ ;
          printf("2: received a no-ops, par_id: %d\n", par_id);
              while (1) {
                  if (sync_util::sync_logger::noops_cnt_hole.load(memory_order_acquire) % nthreads == 0) {
                  printf("2-1: received a no-ops, par_id: %d, cnt: %d\n", par_id, sync_util::sync_logger::noops_cnt_hole.load(memory_order_acquire));
                      break ;
                  } else {
                      std::this_thread::sleep_for(std::chrono::milliseconds(10));
                  }
              }

              sync_util::sync_logger::cg_[par_id].store(0, memory_order_release) ;
              // need to wait until upon all followers's initialization
              sync_util::sync_logger::noops_cnt_init ++ ;
          printf("3: received a no-ops, par_id: %d\n", par_id);
              while (1) {
                  if (sync_util::sync_logger::noops_cnt_init.load(memory_order_acquire) % nthreads == 0) {
                  printf("3-1: received a no-ops, par_id: %d, cnt: %d\n", par_id, sync_util::sync_logger::noops_cnt_init.load(memory_order_acquire));
                      break ;
                  } else {
                      std::this_thread::sleep_for(std::chrono::milliseconds(10));
                  }
              }
              sync_util::sync_logger::computeG() ;
          }
#else
          assert(len % 27 == 0) ;
          //Treplay_in_same_thread_wrapper_pointer_even(par_id, 0, (char *)log, len / 27, tpool.getDBWrapper(par_id)->getDB ());
#endif

        auto endTime = std::chrono::high_resolution_clock::now ();
        TimerMapper::add_time ("replay_wait_"+std::to_string(i), std::chrono::duration_cast<std::chrono::nanoseconds> (endTime - startTime).count (),1000.0 * 1000.0);
#endif
      }

      if (stats) {
          auto time_stamp = std::to_string(static_cast<long int> (std::time(0)));
          _follower_callbacks[time_stamp]++;
#if ALLOW_FOLLOWER_REPLAY
          if(local_end_recv){
            auto str_time_stamp = std::to_string(static_cast<long int> (std::time(0)));
            auto time_stamp = static_cast<long int> (std::time(0));
            std::cout << "[[end]] " << time_stamp << ":" << str_time_stamp << std::endl;
            int _count=0;
            for(auto &each: _follower_callbacks){
                std::cout << _count << " : seconds elapsed " << " Epoch : " << each.first << " : count " << each.second << std::endl;
                _count++;
            }
          }
#endif
      }

      if (local_end_recv) {  // lag stage for each paxos group: at this stage, make sure all logs have to be replayed
         printf("par_id: %d has %lu to be replayed!\n", par_id, un_replay_logs_.size());
         while (un_replay_logs_.size() > 0) {
             auto gg = sync_util::sync_logger::retrieveG() ;
             auto itt = un_replay_logs_.front() ;
              if (std::get<0>(itt) <= gg) {
                  //treplay_in_same_thread_opt(par_id, (char *) std::get<3>(itt), std::get<2>(itt), std::ref(tpool.getDBWrapper(par_id)->getDB ()), table_logger::table_set);
                  //treplay_in_same_thread_opt_mbta(par_id, (char *) std::get<3>(itt), std::get<2>(itt), db, table_logger::table_set);
                  if (consume_running) {
                        redo_buffers[par_id].push_back(std::tuple((char *) std::get<3>(itt), std::get<2>(itt)));
                  }
                  treplay_in_same_thread_opt_mbta_v2(par_id, (char *) std::get<3>(itt), std::get<2>(itt), db);
                  un_replay_logs_.pop() ;
                  free((char*)std::get<3>(itt));
              } else {
                  std::this_thread::sleep_for(std::chrono::microseconds(500));
              }
         }
         //std::cout << "par_id: " << par_id << " has completed: " << un_replay_logs_.size() << std::endl;
      }
      return latest_commit_id * 10 + status ;
      #endif
      }, i);

    register_for_leader_par_id_return([&,i](const char*& log, int len, int par_id, std::queue<std::tuple<unsigned long long int, int, int, const char *>> & un_replay_logs_) {
      #if defined(SINGLE_PAXOS) || defined(DISABLE_REPLAY)
      return 4;
      #else 
#ifdef PAXOS_LEADER_HERE
      auto latest_commit_id = get_latest_commit_id ((char *) log, len) ;
      latest_commit_id = latest_commit_id / 1000 ;
      int epoch = latest_commit_id % 1000 ;
      sync_util::sync_logger::cg_[par_id].store(latest_commit_id, memory_order_release) ;
      bool stats = false ;
      if (stats) {
          auto time_stamp = std::to_string(static_cast<long int> (std::time(0)));
          _leader_callbacks[time_stamp]++;
      }

      uint64_t g = sync_util::sync_logger::retrieveG(true) ;
      if (g != numeric_limits<uint64_t>::max() && g > latestG) {
          latestG = g ;
#if defined(LATENCY)
          advanceGTracker.push_back(std::make_pair(g, timer::cur_usec() )) ;
#endif
      }
#endif
    return 0;
    #endif
    }, i);
  }

  register_getdb_([&]() {
      abstract_db * db = tpool_mbta.getDBWrapper(0)->getDB ();
      //std::cout << "register db: " << db << std::endl;
      return db;
  });

  for (int i=0; i<nthreads; ++i) {
      queue_type q;
      qList.push_back(q) ;
  }

  bool disabled_adding = true;

  // {["localhost"] = "10.1.0.7", ["p1"] = "10.1.0.8", ["p2"] = "10.1.0.9"}
  // leader: 8080, p1: 8081, p2: 8082 for RPC ports
  std::map<std::string, std::string> hosts = getHosts(paxos_config_file[0]) ;  // the first '-F' should contain "host" attribute
  std::map<std::string, std::string>::iterator it;

  if (false && sender > 0 && !disabled_adding) {  // start a thread on the old leader
    thread start_old_leader_server(&older_leader_server);
    pthread_setname_np(start_old_leader_server.native_handle(), "start_old_leader_server");
    start_old_leader_server.detach();

    // it's time to record redo logs in the replay side
    consume_running = true;

    for (it = hosts.begin(); it !=hosts.end(); it++) {
        if (paxos_proc_name.compare(it->first)!=0) {  // paxos_proc_name => "localhost"
            initAddFollowerReplay(nthreads) ;
            // 1. elect the first follower as the sync_source since the follower doesn't have speculative transactions
            sync_util::sync_logger::rpc_sync_host = it->second;
            sync_util::sync_logger::rpc_sync_port = get_server_rpc_port(it->first);
            rpc::client client(it->second, 8080 + get_server_rpc_port(it->first));
            std::cout << "send onGetRole....\n" ;
            auto result = client.call("onGetRole").as<int>();

            if (result == 1) {
                std::cout << "send onSyncSource...\n" ;
                result = client.call("onSyncSource").as<int>();
            } else {
                continue ;
            }

            // 2. wait all scanned logs and redo logs are DONE
            while (completed_consume_threads != nthreads) {
                sleep(1);
                printf("completed_consume_threads: %d/%zu\n", completed_consume_threads, nthreads);
            }

            std::cout << "# of completed threads: " << completed_consume_threads << std::endl;
            // 3. start Paoxs node and then retrieve and replay logs from all Paxos nodes
            // 3.1. start the Paxos nodes as followers to join back?
            int ret2 = setup2(1);
            // 3.2. TODO, retrive Paxos logs
            // 3.3. consensus and replaying logs as normal
            if (!leader_config && multi_process) {
                while (!(count == nthreads && end_recv)) {
                    sleep(1);
                    printf("follower is waiting for being ended: %d/%zu\n", count.load(), nthreads);
                }
             }
             break;
        }
    }
  } else { // start a thread on the follower
      if (false && paxos_proc_name.compare("localhost") != 0 && !disabled_adding) {
          abstract_db * db = tpool_mbta.getDBWrapper(0)->getDB () ;
          thread start_follower_server(&follower_server, db, get_server_rpc_port(paxos_proc_name));
          pthread_setname_np(start_follower_server.native_handle(), "start_follower_server");
          start_follower_server.detach();
      }
  }

  int ret2 = setup2();

  if(!leader_config && multi_process){
      if (bench_type == "tpcc") {
          abstract_db *tdb = NULL;
          tdb = tpool_mbta.getDBWrapper(0)->getDB ();
          modeMonitor(tdb, nthreads) ;
          // mimicMainPaxos() ; // for debug
      }
  }
#endif

    auto dbtest_startTime = std::chrono::high_resolution_clock::now();

    if (leader_config || !multi_process) {
        std::cout << "kSTOBatchSize = " << kSTOBatchSize << std::endl;
#if defined(OPTIMIZED_VERSION_V2)
        //SiloBatching::setSTOBatchSize (kSTOBatchSize);
#else
        StringAllocator::setSTOBatchSize(kSTOBatchSize);
#endif
        std::cout << "nthreads = " << nthreads << std::endl;
    }

    int reret = 0;
    if (leader_config || !multi_process) {
        if (bench_type == "micro") {
            start_Silo_workers_micro(db, nthreads, argc, bench_opts);
        } else if (bench_type == "tpcc") {
            start_SiLo_workers_tpcc(db, nthreads);
        }
        delete db;

        auto dbtest_endTime = std::chrono::high_resolution_clock::now();
        TimerMapper::add_time("dbtest_total_time_spent", std::chrono::duration_cast<std::chrono::nanoseconds>(
                dbtest_endTime - dbtest_startTime).count(), 1000.0 * 1000.0);
#ifdef BENCHMARK_TIME
        for(auto kv : TimerMapper::timer){
            std::cout << "Key=" << kv.first << " Val=" << kv.second/1000.0 << std::endl;
        }
#endif
#ifdef PAXOS_LEADER_HERE
        int _count = 0;
        for (auto &each: _leader_callbacks) {
            std::cout << _count << " : seconds elapsed " << " Epoch : " << each.first << " : count " << each.second
                      << std::endl;
            _count++;
        }
#endif
        std::cout << "total log_size: " << "payment => " << len_payment.load() << ", new_order => "
                  << len_new_order.load() << ", delivery => " << len_delivery.load() << std::endl;
    }

    if (!leader_config && multi_process) {
        while (!(count == nthreads && end_recv)) {
            sleep(1);
            printf("follower is waiting for being ended: %d/%zu\n", count.load(), nthreads);
        }

        /*
        int latency_readyQueue = 0 ;
        for (auto it: readyQueueTracker) {
            if (it > latency_readyQueue)
                latency_readyQueue = it ;
        }
        std::cout << "maximal logs in ready_queue: " << latency_readyQueue << " from total: " << readyQueueTracker.size() << std::endl; */
    }

#if ALLOW_FOLLOWER_REPLAY
    if(!leader_config && multi_process){  // follower
      //dbScan(tpool_mbta.getDBWrapper(0)->getDB ()) ;
      //tpool.closeAll(nthreads) ;
      tpool_mbta.closeAll(nthreads) ;
  } else { // leader
      //tpool.closeAll(nthreads) ;
      tpool_mbta.closeAll(nthreads) ;
      std::cout << "END POINT: " << timeSinceEpochMillisecCommon() << std::endl;
   #if defined DBTEST_PROFILER_ENABLED
     #ifdef USE_JEMALLOC
       std::cout << "#############STOP profiler\n";
       ProfilerStop();
   #elif defined USE_TCMALLOC
       //HeapProfilerStop();
   #endif
 #endif
  }
  sync_util::sync_logger::shutdown() ;
#endif

#if defined(LOG_TO_FILE)
    ref_output["total_number"] = trans_cnt.load() ;
    OutputDataSerializer::GetLogger(9888, LOGGING_CONST::WHOLE_INFO_STRING)->MapWriter(ref_output);
#endif

#if defined(LATENCY)
    if(leader_config || !multi_process) {
        uint64_t latency_ts = 0;
        std::map<unsigned long long int, uint64_t> ordered(sample_transaction_tracker.begin(),
                                                           sample_transaction_tracker.end());
        int valid_cnt = 0;
        std::vector<float> latencyVector ;
        for (auto it: ordered) {  // cid => updated time
            int i = 0;
            for (; i < advanceGTracker.size(); i++) { // G => updated time
                if (advanceGTracker[i].first >= it.first) break;
            }
            if (i < advanceGTracker.size() && advanceGTracker[i].first >= it.first) {
                latency_ts += advanceGTracker[i].second - it.second;
                latencyVector.emplace_back(advanceGTracker[i].second - it.second) ;
                valid_cnt++;
                //std::cout << "Transaction: " << it.first << " takes "
                //          << (advanceGTracker[i].second - it.second) / 1000000.0 << " sec" << std::endl;
            } else { // incur only for last several transactions
                //std::cout << "[no-found] Transaction: " << it.first << std::endl;
            }
        }
        if (latencyVector.size() > 0) {
            //std::cout << "averaged latency: " << latency_ts / 1000.0 / valid_cnt << std::endl;
            std::sort (latencyVector.begin(), latencyVector.end());
            //std::cout << "10% latency: " << latencyVector[(int)(valid_cnt *0.1)] / 1000.0 << std::endl;
            std::cout << "50% latency: " << latencyVector[(int)(valid_cnt *0.5)] / 1000.0 << std::endl;
            std::cout << "90% latency: " << latencyVector[(int)(valid_cnt *0.9)] / 1000.0 << std::endl;
            std::cout << "95% latency: " << latencyVector[(int)(valid_cnt *0.95)] / 1000.0 << std::endl;
            //std::cout << "99% latency: " << latencyVector[(int)(valid_cnt *0.99)] / 1000.0 << std::endl;
        }
    }
#endif

    OutputDataSerializer::flush_all();
    OutputDataSerializer::close_all();

#if defined(PAXOS_LIB_ENABLED)
    std::this_thread::sleep_for(std::chrono::seconds(1));  // left sufficient time to lag stage in paxos followers
    pre_shutdown_step();
    reret =  shutdown_paxos();
#else
    reret = 0;
#endif

#ifdef LOG_TO_FILE
    cout << "-----------------------------\n";
    cout << "Log generation statistic\n";
    cout << "-----------------------------\n";
    std::map<long int, std::pair<long int,long int>> ordered(LogGenRate::counter.begin(),
                                                               LogGenRate::counter.end());
    int _counter=0;
    std::cout << "Sec  \t" << "   unix  \t" << "       \t" << "     \t" << "Average  " << std::endl;
    std::cout << "Spent\t" << "timestamp\t" << "entries\t" << "Bytes\t" << "Entry Size" << std::endl;
    cout << "-----------------------------\n";
    for(auto kv : ordered){
           try{
           std::cout << _counter << "\t" << kv.first << "\t" << kv.second.first ;
                 if ( kv.second.first != 0 && kv.second.second != 0){
                   std::cout << "\t" << kv.second.second/8/1024;
                   std::cout << "\t" << kv.second.second/kv.second.first;
                 }else{
                   std::cout << " <<<<< except ";
                 }
           }catch(...){
           }
           std::cout << std::endl;
           _counter+=1;
    }
#endif

    /*
    #if defined DBTEST_PROFILER_ENABLED
      #ifdef USE_JEMALLOC
          ProfilerStop();
      #elif defined USE_TCMALLOC
          //HeapProfilerStop();
      #endif
    #endif */

    return reret;
}



