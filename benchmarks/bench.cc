#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <utility>
#include <string>
#include <stdlib.h>

#include <stdlib.h>
#include <sched.h>
#include <unistd.h>
#include <sys/sysinfo.h>

#include "bench.h"

#include "../counter.h"
#include "../scopedperf.hh"
#include "../allocator.h"
#include "sto/Transaction.hh"

#ifdef USE_JEMALLOC
//cannot include this header b/c conflicts with malloc.h
//#include <jemalloc/jemalloc.h>
extern "C" void malloc_stats_print(void (*write_cb)(void *, const char *), void *cbopaque, const char *opts);
extern "C" int mallctl(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);
#endif
#ifdef USE_TCMALLOC
#include <google/heap-profiler.h>
#endif

using namespace std;
using namespace util;

size_t nthreads = 1;
volatile bool running = true;
int verbose = 0;
uint64_t txn_flags = 0;
double scale_factor = 1.0;
uint64_t runtime = 30;
uint64_t ops_per_worker = 0;
int run_mode = RUNMODE_TIME;
int enable_parallel_loading = false;
int pin_cpus = 0;
int slow_exit = 0;
int retry_aborted_transaction = 0;
int no_reset_counters = 0;
int backoff_aborted_transaction = 0;
int use_hashtable = 0;

template <typename T>
static void
delete_pointers(const vector<T *> &pts)
{
  for (size_t i = 0; i < pts.size(); i++)
    delete pts[i];
}

template <typename T>
static vector<T>
elemwise_sum(const vector<T> &a, const vector<T> &b)
{
  INVARIANT(a.size() == b.size());
  vector<T> ret(a.size());
  for (size_t i = 0; i < a.size(); i++)
    ret[i] = a[i] + b[i];
  return ret;
}

template <typename K, typename V>
static void
map_agg(map<K, V> &agg, const map<K, V> &m)
{
  for (typename map<K, V>::const_iterator it = m.begin();
       it != m.end(); ++it)
    agg[it->first] += it->second;
}

// returns <free_bytes, total_bytes>
static pair<uint64_t, uint64_t>
get_system_memory_info()
{
  struct sysinfo inf;
  sysinfo(&inf);
  return make_pair(inf.mem_unit * inf.freeram, inf.mem_unit * inf.totalram);
}

static bool
clear_file(const char *name)
{
  ofstream ofs(name);
  ofs.close();
  return true;
}

uint64_t timeSinceEpochMillisecBench() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

static void
write_cb(void *p, const char *s) UNUSED;
static void
write_cb(void *p, const char *s)
{
  const char *f = "jemalloc.stats";
  static bool s_clear_file UNUSED = clear_file(f);
  ofstream ofs(f, ofstream::app);
  ofs << s;
  ofs.flush();
  ofs.close();
}

static event_avg_counter evt_avg_abort_spins("avg_abort_spins");

void
bench_worker::run()
{
  // XXX(stephentu): so many nasty hacks here. should actually
  // fix some of this stuff one day
  if (set_core_id)
    coreid::set_core_id(worker_id); // cringe
  {
    scoped_rcu_region r; // register this thread in rcu region
  }
  on_run_setup();
  scoped_db_thread_ctx ctx(db, false);
  const workload_desc_vec workload = get_workload();
  txn_counts.resize(workload.size());
  barrier_a->count_down();
  barrier_b->wait_for();
  while (running && (run_mode != RUNMODE_OPS || ntxn_commits < ops_per_worker)) {
    double d = r.next_uniform();
    for (size_t i = 0; i < workload.size(); i++) {
      if ((i + 1) == workload.size() || d < workload[i].frequency) {
      retry:
        timer t;
        const unsigned long old_seed = r.get_seed();
        const auto ret = workload[i].fn(this);
        if (likely(ret.first)) {
          ++ntxn_commits;
          latency_numer_us += t.lap();
          backoff_shifts >>= 1;
        } else {
          ++ntxn_aborts;
          if (retry_aborted_transaction && running) {
            if (backoff_aborted_transaction) {
              if (backoff_shifts < 63)
                backoff_shifts++;
              uint64_t spins = 1UL << backoff_shifts;
              spins *= 100; // XXX: tuned pretty arbitrarily
              evt_avg_abort_spins.offer(spins);
              while (spins) {
                nop_pause();
                spins--;
              }
            }
            r.set_seed(old_seed);
            goto retry;
          }
        }
        size_delta += ret.second; // should be zero on abort
        txn_counts[i]++; // txn_counts aren't used to compute throughput (is
                         // just an informative number to print to the console
                         // in verbose mode)
        break;
      }
      d -= workload[i].frequency;
    }
  }
}

void
bench_runner::run()
{
  // load data
  const vector<bench_loader *> loaders = make_loaders();
  {
    spin_barrier b(loaders.size());
    const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();
    {
      scoped_timer t("dataloading", verbose);
      for (vector<bench_loader *>::const_iterator it = loaders.begin();
          it != loaders.end(); ++it) {
        (*it)->set_barrier(b);
        (*it)->start();
      }
      for (vector<bench_loader *>::const_iterator it = loaders.begin();
          it != loaders.end(); ++it)
        (*it)->join();
    }
    const pair<uint64_t, uint64_t> mem_info_after = get_system_memory_info();
    const int64_t delta = int64_t(mem_info_before.first) - int64_t(mem_info_after.first); // free mem
    const double delta_mb = double(delta)/1048576.0;
    if (verbose)
      cerr << "DB size: " << delta_mb << " MB" << endl;
  }

  db->do_txn_epoch_sync(); // also waits for worker threads to be persisted
  {
    const auto persisted_info = db->get_ntxn_persisted();
    if (get<0>(persisted_info) != get<1>(persisted_info))
      cerr << "ERROR: " << persisted_info << endl;
    //ALWAYS_ASSERT(get<0>(persisted_info) == get<1>(persisted_info));
    if (verbose)
      cerr << persisted_info << " txns persisted in loading phase" << endl;
  }
  db->reset_ntxn_persisted();

  if (!no_reset_counters) {
    event_counter::reset_all_counters(); // XXX: for now - we really should have a before/after loading
    PERF_EXPR(scopedperf::perfsum_base::resetall());
  }
  {
    const auto persisted_info = db->get_ntxn_persisted();
    if (get<0>(persisted_info) != 0 ||
        get<1>(persisted_info) != 0 ||
        get<2>(persisted_info) != 0.0) {
      cerr << persisted_info << endl;
      ALWAYS_ASSERT(false);
    }
  }

  map<string, size_t> table_sizes_before;
  if (verbose) {
    for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
         it != open_tables.end(); ++it) {
      scoped_rcu_region guard;
      const size_t s = it->second->size();
      cerr << "table " << it->first << " size " << s << endl;
      table_sizes_before[it->first] = s;
    }
    cerr << "starting benchmark..." << endl;
  }

  const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();

  const vector<bench_worker *> workers = make_workers();
  ALWAYS_ASSERT(!workers.empty());
  Transaction::clear_stats();

  //core binding
  int cpu_id = 0;
  int base = 0;

  for (vector<bench_worker *>::const_iterator it = workers.begin();
       it != workers.end(); ++it) {
    if(cpu_gap) {
      (*it)->start(cpu_id);
      cpu_id += cpu_gap;
      if(cpu_id >= num_cpus) {
	base++;
	cpu_id = base;
      }
    } else {
      (*it)->start();
    }
  }
    
  barrier_a.wait_for(); // wait for all threads to start up
  timer t, t_nosync;
  barrier_b.count_down(); // bombs away!
  if (run_mode == RUNMODE_TIME) {
    sleep(runtime);
    running = false;
  }
  __sync_synchronize();
  for (size_t i = 0; i < nthreads; i++)
    workers[i]->join();
  const unsigned long elapsed_nosync = t_nosync.lap();
  db->do_txn_finish(); // waits for all worker txns to persist
  //  usleep(100000);
  size_t n_commits = 0;
  size_t n_aborts = 0;
  uint64_t latency_numer_us = 0;
  for (size_t i = 0; i < nthreads; i++) {
    n_commits += workers[i]->get_ntxn_commits();
    n_aborts += workers[i]->get_ntxn_aborts();
    latency_numer_us += workers[i]->get_latency_numer_us();
  }
  const auto persisted_info = db->get_ntxn_persisted();

  const unsigned long elapsed = t.lap(); // lap() must come after do_txn_finish(),
                                         // because do_txn_finish() potentially
                                         // waits a bit

  // various sanity checks
  ALWAYS_ASSERT(get<0>(persisted_info) == get<1>(persisted_info));
  // not == b/c persisted_info does not count read-only txns
  ALWAYS_ASSERT(n_commits >= get<1>(persisted_info));

  const double elapsed_nosync_sec = double(elapsed_nosync) / 1000000.0;
  const double agg_nosync_throughput = double(n_commits) / elapsed_nosync_sec;
  const double avg_nosync_per_core_throughput = agg_nosync_throughput / double(workers.size());

  const double elapsed_sec = double(elapsed) / 1000000.0;
  const double agg_throughput = double(n_commits) / elapsed_sec;
  const double avg_per_core_throughput = agg_throughput / double(workers.size());

  const double agg_abort_rate = double(n_aborts) / elapsed_sec;
  const double avg_per_core_abort_rate = agg_abort_rate / double(workers.size());

  // we can use n_commits here, because we explicitly wait for all txns
  // run to be durable
  const double agg_persist_throughput = double(n_commits) / elapsed_sec;
  const double avg_per_core_persist_throughput =
    agg_persist_throughput / double(workers.size());

  // XXX(stephentu): latency currently doesn't account for read-only txns
  const double avg_latency_us =
    double(latency_numer_us) / double(n_commits);
  const double avg_latency_ms = avg_latency_us / 1000.0;
  const double avg_persist_latency_ms =
    get<2>(persisted_info) / 1000.0;

  map<string, size_t> agg_txn_counts = workers[0]->get_txn_counts();
  ssize_t size_delta = workers[0]->get_size_delta();
  for (size_t i = 1; i < workers.size(); i++) {
    map_agg(agg_txn_counts, workers[i]->get_txn_counts());
    size_delta += workers[i]->get_size_delta();
  }

  if (verbose) {
    const pair<uint64_t, uint64_t> mem_info_after = get_system_memory_info();
    const int64_t delta = int64_t(mem_info_before.first) - int64_t(mem_info_after.first); // free mem
    const double delta_mb = double(delta)/1048576.0;
    const double size_delta_mb = double(size_delta)/1048576.0;
    map<string, counter_data> ctrs = event_counter::get_all_counters();

    cerr << "--- table statistics ---" << endl;
    for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
         it != open_tables.end(); ++it) {
      scoped_rcu_region guard;
      const size_t s = it->second->size();
      const ssize_t delta = ssize_t(s) - ssize_t(table_sizes_before[it->first]);
      cerr << "table " << it->first << " size " << it->second->size();
      if (delta < 0)
        cerr << " (" << delta << " records)" << endl;
      else
        cerr << " (+" << delta << " records)" << endl;
    } 
#ifdef ENABLE_BENCH_TXN_COUNTERS
    cerr << "--- txn counter statistics ---" << endl;
    {
      // take from thread 0 for now
      abstract_db::txn_counter_map agg = workers[0]->get_local_txn_counters();
      for (auto &p : agg) {
        cerr << p.first << ":" << endl;
        for (auto &q : p.second)
          cerr << "  " << q.first << " : " << q.second << endl;
      }
    }
#endif
    cerr << "--- benchmark statistics ---" << endl;
    cerr << "runtime: " << elapsed_sec << " sec" << endl;
    cerr << "memory delta: " << delta_mb  << " MB" << endl;
    cerr << "memory delta rate: " << (delta_mb / elapsed_sec)  << " MB/sec" << endl;
    cerr << "logical memory delta: " << size_delta_mb << " MB" << endl;
    cerr << "logical memory delta rate: " << (size_delta_mb / elapsed_sec) << " MB/sec" << endl;
    cerr << "agg_nosync_throughput: " << agg_nosync_throughput << " ops/sec" << endl;
    cerr << "avg_nosync_per_core_throughput: " << avg_nosync_per_core_throughput << " ops/sec/core" << endl;
    cerr << "agg_throughput: " << agg_throughput << " ops/sec" << endl;
    cerr << "avg_per_core_throughput: " << avg_per_core_throughput << " ops/sec/core" << endl;
    cerr << "agg_persist_throughput: " << agg_persist_throughput << " ops/sec" << endl;
    cerr << "avg_per_core_persist_throughput: " << avg_per_core_persist_throughput << " ops/sec/core" << endl;
    cerr << "avg_latency: " << avg_latency_ms << " ms" << endl;
    cerr << "avg_persist_latency: " << avg_persist_latency_ms << " ms" << endl;
    cerr << "agg_abort_rate: " << agg_abort_rate << " aborts/sec" << endl;
    cerr << "avg_per_core_abort_rate: " << avg_per_core_abort_rate << " aborts/sec/core" << endl;
    cerr << "txn breakdown: " << format_list(agg_txn_counts.begin(), agg_txn_counts.end()) << endl;
    cerr << "--- system counters (for benchmark) ---" << endl;
    for (map<string, counter_data>::iterator it = ctrs.begin();
         it != ctrs.end(); ++it)
      cerr << it->first << ": " << it->second << endl;
    cerr << "--- perf counters (if enabled, for benchmark) ---" << endl;
    PERF_EXPR(scopedperf::perfsum_base::printall());
    cerr << "--- allocator stats ---" << endl;
    ::allocator::DumpStats();
    cerr << "---------------------------------------" << endl;

#ifdef USE_JEMALLOC
    cerr << "dumping heap profile..." << endl;
    mallctl("prof.dump", NULL, NULL, NULL, 0);
    cerr << "printing jemalloc stats..." << endl;
    //malloc_stats_print(write_cb, NULL, "");
#endif
#ifdef USE_TCMALLOC
    HeapProfilerDump("before-exit");
#endif
  }

  // output for plotting script
  cout << fixed << agg_throughput << " "
       << agg_persist_throughput << " "
       << avg_latency_ms << " "
       << avg_persist_latency_ms << " "
       << agg_abort_rate << " "
       << agg_txn_counts["NewOrder"] << endl;
  cout.flush();

  for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
       it != open_tables.end(); ++it) {
    //it->second->print_stats();
  }

  if (!slow_exit)
    return;

  map<string, uint64_t> agg_stats;
  for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
       it != open_tables.end(); ++it) {
    map_agg(agg_stats, it->second->clear());
    delete it->second;
  }
  if (verbose) {
    for (auto &p : agg_stats)
      cerr << p.first << " : " << p.second << endl;

  }
  open_tables.clear();

  delete_pointers(loaders);
  delete_pointers(workers);
}

void
bench_runner::run_without_stats()
{
  const vector<bench_loader *> loaders = make_loaders();
  if (f_mode == 0) { // weihai, f_mode==0 is normal to load data
      {
          std::cout << "Load phase, load_size: " << loaders.size() << std::endl;
          spin_barrier b(loaders.size());
          const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();
          {
              scoped_timer t("dataloading", verbose);
              for (vector<bench_loader *>::const_iterator it = loaders.begin();
                   it != loaders.end(); ++it) {
                  (*it)->set_barrier(b);
                  (*it)->start();
              }
              for (vector<bench_loader *>::const_iterator it = loaders.begin();
                   it != loaders.end(); ++it)
                  (*it)->join();
          }
          const pair<uint64_t, uint64_t> mem_info_after = get_system_memory_info();
          const int64_t delta = int64_t(mem_info_before.first) - int64_t(mem_info_after.first); // free mem
          const double delta_mb = double(delta)/1048576.0;
          if (verbose)
              cerr << "DB size: " << delta_mb << " MB" << endl;
      }

      db->do_txn_epoch_sync(); // also waits for worker threads to be persisted
      {
          const auto persisted_info = db->get_ntxn_persisted();
          if (get<0>(persisted_info) != get<1>(persisted_info))
              cerr << "ERROR: " << persisted_info << endl;
          //ALWAYS_ASSERT(get<0>(persisted_info) == get<1>(persisted_info));
          if (verbose)
              cerr << persisted_info << " txns persisted in loading phase" << endl;
      }
      db->reset_ntxn_persisted();

      if (!no_reset_counters) {
          event_counter::reset_all_counters(); // XXX: for now - we really should have a before/after loading
          PERF_EXPR(scopedperf::perfsum_base::resetall());
      }
      {
          const auto persisted_info = db->get_ntxn_persisted();
          if (get<0>(persisted_info) != 0 ||
              get<1>(persisted_info) != 0 ||
              get<2>(persisted_info) != 0.0) {
              cerr << persisted_info << endl;
              ALWAYS_ASSERT(false);
          }
      }
  } else if (f_mode == 1) {  // without load phase

  } else {
      ALWAYS_ASSERT(false) ;
  }

  map<string, size_t> table_sizes_before;
  if (verbose) {
//    for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
//         it != open_tables.end(); ++it) {
//      scoped_rcu_region guard;
//      const size_t s = it->second->size();
//      cerr << "table " << it->first << " size " << s << endl;
//      table_sizes_before[it->first] = s;
//    }
    cerr << "starting benchmark...." << endl;
  }

  if (f_mode == 0) {  // ONLY normal case need this
      sleep(nthreads);
#if defined(NETWORK_CLIENT) || defined(NETWORK_CLIENT_YCSB)
    //nc_setup_server(nthreads, "10.1.0.7");
    nc_setup_server(nthreads, "127.0.0.1");
#endif
  }

  len_payment = 0;
  len_new_order = 0;
  len_delivery = 0;

  bool run_workers = true ;
  if (run_workers) {
      const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();
      const vector<bench_worker *> workers = make_workers();
      ALWAYS_ASSERT(!workers.empty());
      Transaction::clear_stats();

      //core binding
      int cpu_id = 0;
      int base = 0;

      std::cout << "[info]bench worker is working on # of num_cpus " << num_cpus << ", cpu_gap " << cpu_gap << std::endl;
      for (vector<bench_worker *>::const_iterator it = workers.begin();
           it != workers.end(); ++it) {
          if(cpu_gap) {
              (*it)->start(cpu_id);
              cpu_id += cpu_gap;
              if(cpu_id >= num_cpus) {
                  base++;
                  cpu_id = base;
              }
          } else {
              (*it)->start();
          }
      }

      barrier_a.wait_for(); // wait for all threads to start up
      // missing t,t_nosync
      _t.init();
      _t_nosync.init();

      barrier_b.count_down(); // bombs away!
      uint64_t n_commits_5 = 0 ;
      uint64_t n_commits_25 = 0 ;
      if (run_mode == RUNMODE_TIME) {
          #if defined(FAIL_OVER_VARIABLE)
              bool fail = true;
          #else
              bool fail = false;
          std::cout << "START POINT: " << timeSinceEpochMillisecBench() << ", fail: " << fail << std::endl;
          #endif
          for(uint64_t i=0; i<runtime*10; i++) {
              usleep(100000);
              if (fail && i == 10*10) {
                 system("sudo pkill -f dbtest");
                 exit(0);
              }
              uint64_t n_commits = 0 ;

              for (size_t j = 0; j < nthreads; j++) {
                  n_commits += workers[j]->get_ntxn_commits();
		  //std::cout << "time: " << j << ": " << workers[j]->get_ntxn_commits() << std::endl;
              }
              if (i==4*10) n_commits_5 = n_commits;
              if (i==24*10) n_commits_25 = n_commits;
              std::cout << "[time: " << timeSinceEpochMillisecBench() << "] # of commits: " << n_commits  << std::endl;
          }
          running = false;
      }
      printf("[agg_throughput-5-25] [%lu-%lu]throughput without warmup and cool-down: %f\n", n_commits_25, n_commits_5, (n_commits_25 - n_commits_5) / 20.0);
      __sync_synchronize();
      std::cout << "finish __sync_synchronize\n";
      for (size_t i = 0; i < nthreads; i++) {
          std::cout << "wait for worker: " << i << std::endl;
          workers[i]->join();
          std::cout << "complete for worker: " << i << std::endl;
      }
          
      const unsigned long elapsed_nosync = _t_nosync.lap();
      db->do_txn_finish(); // waits for all worker txns to persist
      //  usleep(100000);

      temp_info_.loaders = loaders;
      temp_info_.workers = workers;
      temp_info_.elapsed_nosync = elapsed_nosync;
      temp_info_.mem_info_before = mem_info_before;
      temp_info_.table_sizes_before = table_sizes_before;
  }
}

std::map<std::string, abstract_ordered_index *>
bench_runner::get_open_tables() {
    return open_tables;
}

void
bench_runner::print_stats()
{
  const auto loaders = temp_info_.loaders;
  auto workers = temp_info_.workers;
  auto elapsed_nosync = temp_info_.elapsed_nosync;
  auto mem_info_before = temp_info_.mem_info_before;
  auto table_sizes_before = temp_info_.table_sizes_before;

  /*
  #if 0
  // load data
  const vector<bench_loader *> loaders = make_loaders();
  {
    spin_barrier b(loaders.size());
    const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();
    {
      scoped_timer t("dataloading", verbose);
      for (vector<bench_loader *>::const_iterator it = loaders.begin();
          it != loaders.end(); ++it) {
        (*it)->set_barrier(b);
        (*it)->start();
      }
      for (vector<bench_loader *>::const_iterator it = loaders.begin();
          it != loaders.end(); ++it)
        (*it)->join();
    }
    const pair<uint64_t, uint64_t> mem_info_after = get_system_memory_info();
    const int64_t delta = int64_t(mem_info_before.first) - int64_t(mem_info_after.first); // free mem
    const double delta_mb = double(delta)/1048576.0;
    if (verbose)
      cerr << "DB size: " << delta_mb << " MB" << endl;
  }

  db->do_txn_epoch_sync(); // also waits for worker threads to be persisted
  {
    const auto persisted_info = db->get_ntxn_persisted();
    if (get<0>(persisted_info) != get<1>(persisted_info))
      cerr << "ERROR: " << persisted_info << endl;
    //ALWAYS_ASSERT(get<0>(persisted_info) == get<1>(persisted_info));
    if (verbose)
      cerr << persisted_info << " txns persisted in loading phase" << endl;
  }
  db->reset_ntxn_persisted();

  if (!no_reset_counters) {
    event_counter::reset_all_counters(); // XXX: for now - we really should have a before/after loading
    PERF_EXPR(scopedperf::perfsum_base::resetall());
  }
  {
    const auto persisted_info = db->get_ntxn_persisted();
    if (get<0>(persisted_info) != 0 ||
        get<1>(persisted_info) != 0 ||
        get<2>(persisted_info) != 0.0) {
      cerr << persisted_info << endl;
      ALWAYS_ASSERT(false);
    }
  }

  map<string, size_t> table_sizes_before;
  if (verbose) {
    for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
         it != open_tables.end(); ++it) {
      scoped_rcu_region guard;
      const size_t s = it->second->size();
      cerr << "table " << it->first << " size " << s << endl;
      table_sizes_before[it->first] = s;
    }
    cerr << "starting benchmark..." << endl;
  }

  const pair<uint64_t, uint64_t> mem_info_before = get_system_memory_info();

  const vector<bench_worker *> workers = make_workers();
  ALWAYS_ASSERT(!workers.empty());
  Transaction::clear_stats();

  //core binding
  int cpu_id = 0;
  int base = 0;

  for (vector<bench_worker *>::const_iterator it = workers.begin();
       it != workers.end(); ++it) {
    if(cpu_gap) {
      (*it)->start(cpu_id);
      cpu_id += cpu_gap;
      if(cpu_id >= num_cpus) {
	base++;
	cpu_id = base;
      }
    } else {
      (*it)->start();
    }
  }
  
  barrier_a.wait_for(); // wait for all threads to start up
  timer t, t_nosync;
  barrier_b.count_down(); // bombs away!
  if (run_mode == RUNMODE_TIME) {
    sleep(runtime);
    running = false;
  }
  __sync_synchronize();
  for (size_t i = 0; i < nthreads; i++)
    workers[i]->join();
  const unsigned long elapsed_nosync = t_nosync.lap();
  db->do_txn_finish(); // waits for all worker txns to persist
  //  usleep(100000);
  #endif  */
  // workers
  // elapsed_nosync
  // loaders
  
  size_t n_commits = 0;
  size_t n_commits_batch = 0;
  size_t n_random_time_taken = 0;
  size_t n_random_time_taken1 = 0;
  size_t n_random_time_taken2 = 0;
  size_t n_random_time_taken3 = 0;
  size_t n_random_time_taken4 = 0;
  size_t n_random_time_taken5 = 0;
  size_t n_aborts = 0;
  uint64_t latency_numer_us = 0;
  for (size_t i = 0; i < nthreads; i++) {
    n_commits += workers[i]->get_ntxn_commits();
    n_commits_batch += workers[i]->get_ntxn_commits_batch();
    n_random_time_taken += workers[i]->get_random_time_taken();
    n_random_time_taken1 += workers[i]->get_random_time_taken1();
    n_random_time_taken2 += workers[i]->get_random_time_taken2();
    n_random_time_taken3 += workers[i]->get_random_time_taken3();
    n_random_time_taken4 += workers[i]->get_random_time_taken4();
    n_random_time_taken5 += workers[i]->get_random_time_taken5();
    n_aborts += workers[i]->get_ntxn_aborts();
    latency_numer_us += workers[i]->get_latency_numer_us();
    //std::cout << "thread_id " << i << " latency is " <<  workers[i]->get_latency_numer_us() / 1000.0 << " ms n_commits is \t " << workers[i]->get_ntxn_commits() << std::endl;
  }
  const auto persisted_info = db->get_ntxn_persisted();

  const unsigned long elapsed = _t.lap(); // lap() must come after do_txn_finish(),
                                         // because do_txn_finish() potentially
                                         // waits a bit

  // various sanity checks
  ALWAYS_ASSERT(get<0>(persisted_info) == get<1>(persisted_info));
  // not == b/c persisted_info does not count read-only txns
  ALWAYS_ASSERT(n_commits >= get<1>(persisted_info));

  const double elapsed_nosync_sec = double(elapsed_nosync) / 1000000.0;
  const double agg_nosync_throughput = double(n_commits) / elapsed_nosync_sec;
  const double avg_nosync_per_core_throughput = agg_nosync_throughput / double(workers.size());

  const double elapsed_sec = double(elapsed) / 1000000.0;
  const double agg_throughput = double(n_commits) / elapsed_sec;
  const double avg_per_core_throughput = agg_throughput / double(workers.size());

  const double agg_abort_rate = double(n_aborts) / elapsed_sec;
  const double avg_per_core_abort_rate = agg_abort_rate / double(workers.size());

  // we can use n_commits here, because we explicitly wait for all txns
  // run to be durable
  const double agg_persist_throughput = double(n_commits) / elapsed_sec;
  const double avg_per_core_persist_throughput =
    agg_persist_throughput / double(workers.size());

  // XXX(stephentu): latency currently doesn't account for read-only txns
  const double avg_latency_us =
    double(latency_numer_us) / double(n_commits);
  const double avg_latency_ms = avg_latency_us / 1000.0;
  const double avg_persist_latency_ms =
    get<2>(persisted_info) / 1000.0;

  map<string, size_t> agg_txn_counts = workers[0]->get_txn_counts();
  ssize_t size_delta = workers[0]->get_size_delta();
  for (size_t i = 1; i < workers.size(); i++) {
    map_agg(agg_txn_counts, workers[i]->get_txn_counts());
    size_delta += workers[i]->get_size_delta();
  }

  if (verbose) {
    const pair<uint64_t, uint64_t> mem_info_after = get_system_memory_info();
    const int64_t delta = int64_t(mem_info_before.first) - int64_t(mem_info_after.first); // free mem
    const double delta_mb = double(delta)/1048576.0;
    const double size_delta_mb = double(size_delta)/1048576.0;
    map<string, counter_data> ctrs = event_counter::get_all_counters();

    /*
    cerr << "--- table statistics ---" << endl;
    for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
         it != open_tables.end(); ++it) {
      scoped_rcu_region guard;
      const size_t s = it->second->size();
      const ssize_t delta = ssize_t(s) - ssize_t(table_sizes_before[it->first]);
      cerr << "table " << it->first << " size " << it->second->size();
      if (delta < 0)
        cerr << " (" << delta << " records)" << endl;
      else
        cerr << " (+" << delta << " records)" << endl;
    } */
#ifdef ENABLE_BENCH_TXN_COUNTERS
    cerr << "--- txn counter statistics ---" << endl;
    {
      // take from thread 0 for now
      abstract_db::txn_counter_map agg = workers[0]->get_local_txn_counters();
      for (auto &p : agg) {
        cerr << p.first << ":" << endl;
        for (auto &q : p.second)
          cerr << "  " << q.first << " : " << q.second << endl;
      }
    }
#endif
    cerr << "--- benchmark statistics in print_stats ---" << endl;
    cerr << "runtime: " << elapsed_sec << " sec" << endl;
    cerr << "memory delta: " << delta_mb  << " MB" << endl;
    cerr << "memory delta rate: " << (delta_mb / elapsed_sec)  << " MB/sec" << endl;
    cerr << "logical memory delta: " << size_delta_mb << " MB" << endl;
    cerr << "logical memory delta rate: " << (size_delta_mb / elapsed_sec) << " MB/sec" << endl;
    cerr << "agg_nosync_throughput: " << agg_nosync_throughput << " ops/sec" << endl;
    cerr << "avg_nosync_per_core_throughput: " << avg_nosync_per_core_throughput << " ops/sec/core" << endl;
    cerr << "agg_throughput: " << agg_throughput << " ops/sec" << endl;
    cerr << "random time Taken: " << n_random_time_taken/(nthreads+0.0)/1000000.0 << " seconds" << endl;
    cerr << "random time Taken1: " << n_random_time_taken1/(nthreads+0.0)/1000000.0 << " seconds" << endl;
    cerr << "random time Taken2: " << n_random_time_taken2/(nthreads+0.0)/1000000.0 << " seconds" << endl;
    cerr << "random time Taken3: " << n_random_time_taken3/(nthreads+0.0)/1000000.0 << " seconds" << endl;
    cerr << "random time Taken4: " << n_random_time_taken4/(nthreads+0.0)/1000000.0 << " seconds" << endl;
    cerr << "random time Taken5: " << n_random_time_taken5/(nthreads+0.0)/1000000.0 << " seconds" << endl;
    cerr << "n_commits: " << n_commits << ", n_commits_batch: " << n_commits_batch << " , elapsed: " << elapsed_sec << endl;
    cerr << "avg_per_core_throughput: " << avg_per_core_throughput << " ops/sec/core" << endl;
    cerr << "agg_persist_throughput: " << agg_persist_throughput << " ops/sec" << endl;
    cerr << "avg_per_core_persist_throughput: " << avg_per_core_persist_throughput << " ops/sec/core" << endl;
    cerr << "avg_latency: " << avg_latency_ms << " ms" << endl;
    cerr << "avg_persist_latency: " << avg_persist_latency_ms << " ms" << endl;
    cerr << "agg_abort_rate: " << agg_abort_rate << " aborts/sec" << endl;
    cerr << "avg_per_core_abort_rate: " << avg_per_core_abort_rate << " aborts/sec/core" << endl;
    cerr << "txn breakdown: " << format_list(agg_txn_counts.begin(), agg_txn_counts.end()) << endl;
    cerr << "--- system counters (for benchmark) ---" << endl;
    for (map<string, counter_data>::iterator it = ctrs.begin();
         it != ctrs.end(); ++it)
      cerr << it->first << ": " << it->second << endl;
    cerr << "--- perf counters (if enabled, for benchmark) ---" << endl;
    PERF_EXPR(scopedperf::perfsum_base::printall());
    cerr << "--- allocator stats ---" << endl;
    ::allocator::DumpStats();
    cerr << "---------------------------------------" << endl;

#ifdef USE_JEMALLOC
    cerr << "dumping heap profile..." << endl;
    mallctl("prof.dump", NULL, NULL, NULL, 0);
    cerr << "printing jemalloc stats..." << endl;
    //malloc_stats_print(write_cb, NULL, "");
#endif
#ifdef USE_TCMALLOC
    HeapProfilerDump("before-exit");
#endif
  }

  // output for plotting script
  cout << fixed << agg_throughput << " "
       << agg_persist_throughput << " "
       << avg_latency_ms << " "
       << avg_persist_latency_ms << " "
       << agg_abort_rate << " "
       << agg_txn_counts["NewOrder"] << endl;
  cout.flush();

  for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
       it != open_tables.end(); ++it) {
    //it->second->print_stats();
  }

  if (!slow_exit)
    return;

  map<string, uint64_t> agg_stats;
  for (map<string, abstract_ordered_index *>::iterator it = open_tables.begin();
       it != open_tables.end(); ++it) {
    map_agg(agg_stats, it->second->clear());
    delete it->second;
  }
  if (verbose) {
    for (auto &p : agg_stats)
      cerr << p.first << " : " << p.second << endl;

  }
  open_tables.clear();

  delete_pointers(loaders);
  delete_pointers(workers);
}

template <typename K, typename V>
struct map_maxer {
  typedef map<K, V> map_type;
  void
  operator()(map_type &agg, const map_type &m) const
  {
    for (typename map_type::const_iterator it = m.begin();
        it != m.end(); ++it)
      agg[it->first] = std::max(agg[it->first], it->second);
  }
};

//template <typename KOuter, typename KInner, typename VInner>
//struct map_maxer<KOuter, map<KInner, VInner>> {
//  typedef map<KInner, VInner> inner_map_type;
//  typedef map<KOuter, inner_map_type> map_type;
//};

#ifdef ENABLE_BENCH_TXN_COUNTERS
void
bench_worker::measure_txn_counters(void *txn, const char *txn_name)
{
  auto ret = db->get_txn_counters(txn);
  map_maxer<string, uint64_t>()(local_txn_counters[txn_name], ret);
}
#endif

map<string, size_t>
bench_worker::get_txn_counts() const
{
  map<string, size_t> m;
  const workload_desc_vec workload = get_workload();
  for (size_t i = 0; i < txn_counts.size(); i++)
    m[workload[i].name] = txn_counts[i];
  return m;
}
