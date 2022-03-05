
#include "__dep__.h"
#include "frame.h"
#include "paxos_worker.h"
#include "client_worker.h"
#include "procedure.h"
#include "command_marshaler.h"
#include "benchmark_control_rpc.h"
#include "server_worker.h"
#include "concurrentqueue.h"
#include "sys/time.h"
#ifdef CPU_PROFILE
# include <gperftools/profiler.h>
#endif // ifdef CPU_PROFILE
#include "config.h"
#include "s_main.h"
#include "paxos/server.h"
#include "network_client/network_impl.h"

using namespace janus;
using namespace network_client;

// network client
std::vector<shared_ptr<network_client::NetworkClientServiceImpl>> nc_services = {};
std::vector<shared_ptr<pthread_t>> nc_service_pthreads = {};
// end of network client

vector<unique_ptr<ClientWorker>> client_workers_g = {};
//vector<shared_ptr<PaxosWorker>> pxs_workers_g = {};
//static vector<shared_ptr<Coordinator>> bulk_coord_g = {};
//static vector<pair<string, pair<int,uint32_t>>> submit_loggers(10000000);
typedef std::chrono::high_resolution_clock::time_point tp;
typedef pair<const char*, pair<int,tp>> queue_entry;
typedef pair<const char*, pair<int,int>> queue_entry_par;
static moodycamel::ConcurrentQueue<queue_entry_par> submit_queue;
static std::queue<queue_entry_par> submit_queue_nc;
static rrr::SpinLock l_;
static atomic<int> producer{0}, consumer{0};
static atomic<int> submit_tot{0};
pthread_t submit_poll_th_;
const int len = 5;
static std::map<std::string,long double> timer;

function<void()> leader_callback_{};

std::map<int, std::function<unsigned long long int(const char*&, int, int, std::queue<std::tuple<unsigned long long int, int, int, const char *>> &)>> leader_replay_cb;
std::map<int, std::function<unsigned long long int(const char*&, int, int, std::queue<std::tuple<unsigned long long int, int, int, const char *>> &)>> follower_replay_cb{};


shared_ptr<ElectionState> es = ElectionState::instance();


int get_epoch(){
  int x;
  pxs_workers_g.back()->election_state_lock.lock();
  x = pxs_workers_g.back()->cur_epoch;
  pxs_workers_g.back()->election_state_lock.unlock();
  return x;
}

void check_current_path() {
    auto path = boost::filesystem::current_path();
    Log_info("PWD : %s", path.string().c_str());
}

void server_launch_worker(vector<Config::SiteInfo>& server_sites) {
    auto config = Config::GetConfig();
    
    int i = 0;
    vector<std::thread> setup_ths;
    for (auto& site_info : server_sites) {
        setup_ths.push_back(std::thread([&site_info, &i, &config]() {
            Log_info("launching site: %x, bind address %s",
                     site_info.id,
                     site_info.GetBindAddress().c_str());
            auto& worker = pxs_workers_g[i++];
            //worker->site_info_ = const_cast<Config::SiteInfo*>(&config->SiteById(site_info.id));
            // setup frame and scheduler
            //worker->SetupBase();
            // start server service
            worker->SetupService();
            // setup communicator
            worker->SetupCommo();
            worker->InitQueueRead();
            Log_info("site %d launched!", (int)site_info.id);
        }));
    }
    Log_info("waiting for server setup threads.");
    for (auto& th : setup_ths) {
        th.join();
    }
    Log_info("done waiting for server setup threads.");

    for (auto& worker : pxs_workers_g) {
        // start communicator after all servers are running
        // setup communication between controller script
        worker->SetupHeartbeat();
    }
    Log_info("server workers' communicators setup");
}

char* message[200];
void microbench_paxos() {
    // register callback
    for (auto& worker : pxs_workers_g) {
        if (worker->IsLeader(worker->site_info_->partition_id_))
            worker->register_apply_callback([&worker](const char* log, int len) {
                Log_debug("submit callback enter in");
                if (worker->submit_num >= worker->tot_num) return;
                worker->Submit(log, len, worker->site_info_->partition_id_);
                worker->submit_num++;
            });
        else
            worker->register_apply_callback([=](const char* log, int len) {});
    }
    auto client_infos = Config::GetConfig()->GetMyClients();
    if (client_infos.size() <= 0) return;
    int concurrent = Config::GetConfig()->get_concurrent_txn();
    for (int i = 0; i < concurrent; i++) {
        message[i] = new char[len];
        message[i][0] = (i / 100) + '0';
        message[i][1] = ((i / 10) % 10) + '0';
        message[i][2] = (i % 10) + '0';
        for (int j = 3; j < len - 1; j++) {
            message[i][j] = (rand() % 10) + '0';
        }
        message[i][len - 1] = '\0';
    }
#ifdef CPU_PROFILE
    char prof_file[1024];
  Config::GetConfig()->GetProfilePath(prof_file);
  // start to profile
  ProfilerStart(prof_file);
#endif // ifdef CPU_PROFILE
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    for (int i = 0; i < concurrent; i++) {
        for (auto& worker : pxs_workers_g) {
            worker->Submit(message[i], len, worker->site_info_->partition_id_);
        }
    }
    for (auto& worker : pxs_workers_g) {
        worker->WaitForSubmit();
    }
    gettimeofday(&t2, NULL);
    pxs_workers_g[0]->submit_tot_sec_ += t2.tv_sec - t1.tv_sec;
    pxs_workers_g[0]->submit_tot_usec_ += t2.tv_usec - t1.tv_usec;
#ifdef CPU_PROFILE
    // stop profiling
  ProfilerStop();
#endif // ifdef CPU_PROFILE

    for (int i = 0; i < concurrent; i++) {
        delete message[i];
    }
    Log_info("shutdown Server Control Service after task finish");
    for (auto& worker : pxs_workers_g) {
        if (worker->hb_rpc_server_ != nullptr) {
            worker->scsi_->server_shutdown(nullptr);
        }
    }
}

/*pair<pair<string, pair<int, uint32_t>>, bool> remove_from_submitq(){
  pair<string, pair<int,uint32_t>> paxos_entry;
  bool found = submit_queue.try_dequeue(paxos_entry);
  return make_pair(paxos_entry, found);
}*/

void add_log_without_queue(const char* log, int len, uint32_t par_id){
  char* nlog = (char*)log;
  for (auto& worker : pxs_workers_g) {  // submit a transaction
    if (worker->site_info_->partition_id_ == par_id){
    	    worker->IncSubmit();
          worker->Submit(log,len, par_id);
	    if(es->machine_id == 1){
		Log_debug("Submitted on behalf on new leader %d", (int)worker->n_tot);
	    }
          break;
      }
	    //worker->n_current++;
	    //break;
    }

}


static bool wait = false;
static int count_free = 0;
void submit_logger() {
  auto endTime = std::chrono::high_resolution_clock::now(); 
  pair<const char*, pair<int,int>> paxos_entry;
  auto res = submit_queue.try_dequeue(paxos_entry);
  if(!res){
    return;
  }
  //auto time_in_queue = std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - paxos_entry.second.second).count();
  //free((char*)paxos_entry.first);
  //Log_info("Time spent in queue for this entry is %d", time_in_queue);
//  if(time_in_queue < (int64_t)100*1000*1000){
//	  wait = true;
//  }else{
//	wait = false;
//  }
//  count_free++;
//  return;
  //consumer++;
  const char* nlog = paxos_entry.first;
  int len = paxos_entry.second.first;
  uint32_t par_id = paxos_entry.second.second;
  add_log_without_queue(nlog, len, par_id);
}

void* PollSubmitLog(void* arg){
    while(producer >= 0){
      submit_logger();
      if(wait){
	      //Log_info("The number of entries freed are %d, size of the queue is %d", count_free, submit_queue.size_approx());
	      std::this_thread::sleep_for(std::chrono::milliseconds(100));
	      count_free = 0;
      }
    }
    pthread_exit(nullptr);
    return nullptr;
}

map<string, string> getHosts(std::string filename) {
    map<string, string> proc_host_map_;
    YAML::Node config = YAML::LoadFile(filename);

    if (config["host"]) {
        auto node = config["host"];
        for (auto it = node.begin(); it != node.end(); it++) {
            auto proc_name = it->first.as<string>();
            auto host_name = it->second.as<string>();
            proc_host_map_[proc_name] = host_name ;
        }
    } else {
        std::cout << "there is no host attribute in the XML: " << filename << std::endl;
        exit(1) ;
    }
    return proc_host_map_;
}

int get_outstanding_logs(uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {  // submit a transaction
        if (worker->site_info_->partition_id_ == par_id){
            //if(es->machine_id == 1) {  //  if leader
            return (int)worker->n_tot - (int)worker->n_current ;
            //}
        }
    }
    return -1;
}

int get_outstanding_logs_cur(uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {  // submit a transaction
        if (worker->site_info_->partition_id_ == par_id){
            //if(es->machine_id == 1) {  //  if leader
            return (int)worker->n_current ;
            //}
        }
    }
    return -1;
}

int get_outstanding_logs_tol(uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {  // submit a transaction
        if (worker->site_info_->partition_id_ == par_id){
            //if(es->machine_id == 1) {  //  if leader
            return (int)worker->n_tot ;
            //}
        }
    }
    return -1;
}

int get_outstanding_logs_que(uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {  // submit a transaction
        if (worker->site_info_->partition_id_ == par_id){
            //if(es->machine_id == 1) {  //  if leader
            return (int)worker->replay_queue.size_approx() ;
            //}
        }
    }
    return -1;
}

std::vector<std::string> setup(int argc, char* argv[]) {
    vector<string> retVector;
    check_current_path();
    Log_info("starting process %ld", getpid());

    int ret = Config::CreateConfig(argc, argv);
    if (ret != SUCCESS) {
        Log_fatal("Read config failed");
        return retVector;
    }

    auto server_infos = Config::GetConfig()->GetMyServers();
    Log_info("server enabled, number of sites: %d", server_infos.size());
    for (int i = server_infos.size()-1; i >=0; i--) {
      retVector.push_back(Config::GetConfig()->SiteById(server_infos[i].id).name) ;
      PaxosWorker* worker = new PaxosWorker();
      pxs_workers_g.push_back(std::shared_ptr<PaxosWorker>(worker));
      //std::cout << i << endl;
      pxs_workers_g.back()->site_info_ = const_cast<Config::SiteInfo*>(&(Config::GetConfig()->SiteById(server_infos[i].id)));
      Log_info("parition id of each is %d", pxs_workers_g.back()->site_info_->partition_id_);
      // setup frame and scheduler
      pxs_workers_g.back()->SetupBase();
    }
    reverse(pxs_workers_g.begin(), pxs_workers_g.end());
    es->machine_id = pxs_workers_g.back()->site_info_->locale_id;
    return retVector;
}

int shutdown_paxos() {
    // kill the election thread
    es->running = false;

    for(auto kv : timer){
   	 std::cout << "Key=" << kv.first << " Val=" << kv.second/1000.0 << std::endl;
    }
    for (auto& worker : pxs_workers_g) {
        worker->WaitForShutdown();
    }
    Log_info("all server workers have shut down.");

    fflush(stderr);
    fflush(stdout);

    for (auto& worker : pxs_workers_g) {
        worker->ShutDown();
    }
    pxs_workers_g.clear();

    RandomGenerator::destroy();
    Config::DestroyConfig();

    Log_info("exit process.");

    return 0;
}

void register_leader_election_callback(std::function<void()> cb){
  leader_callback_ = cb;
}

void register_for_follower(std::function<void(const char*, int)> cb, uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {
        if (worker->IsPartition(par_id) && !worker->IsLeader(par_id)) {
            worker->register_apply_callback(cb);
        }
    }
}

void register_for_follower_par_id(std::function<void(const char*&, int, int)> cb, uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {
        if (worker->IsPartition(par_id) && !worker->IsLeader(par_id)) {
            worker->register_apply_callback_par_id(cb);
        }
    }
}

void register_for_follower_par_id_return(std::function<unsigned long long int(const char*&, int, int, std::queue<std::tuple<unsigned long long int, int, int, const char *>> &)> cb, uint32_t par_id) {
    follower_replay_cb[par_id] = cb;
    if(es->machine_id != 0){
      for (auto& worker : pxs_workers_g) {
        if(worker->IsPartition(par_id))
          worker->register_apply_callback_par_id_return(cb);
      }
    }
}

void register_for_leader(std::function<void(const char*, int)> cb, uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {
        if (worker->IsLeader(par_id)) {
            worker->register_apply_callback(cb);
        }
    }
}

void register_for_leader_par_id(std::function<void(const char*&, int, int)> cb, uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {
        if (worker->IsLeader(par_id)) {
            worker->register_apply_callback_par_id(cb);
        }
    }
}

void register_for_leader_par_id_return(std::function<unsigned long long int(const char*&, int, int, std::queue<std::tuple<unsigned long long int, int, int, const char *>> &)> cb, uint32_t par_id) {
    leader_replay_cb[par_id] = cb;
    if(es->machine_id == 0){
      for (auto& worker : pxs_workers_g) {
        if(worker->IsPartition(par_id))
          worker->register_apply_callback_par_id_return(cb);
      }
    }
}

void submit(const char* log, int len, uint32_t par_id) {
    for (auto& worker : pxs_workers_g) {  // submit a transaction
        if (!worker->IsLeader(par_id)) continue;
        verify(worker->submit_pool != nullptr);
        string log_str;
        std::copy(log, log + len, std::back_inserter(log_str));
        worker->IncSubmit();

        auto sp_job = std::make_shared<OneTimeJob>([&worker,log_str,len,par_id] () {
            worker->Submit(log_str.data(),len, par_id);
        });
        worker->GetPollMgr()->add(sp_job);
        submit_tot++;
    }
}
void add_time(std::string key, long double value,long double denom){
  value /= denom;
  if(timer.find(key)==timer.end()){
    timer[key] = value;
  }else{
    timer[key]+=value;
  }
}

static tp firstTime;
static tp endTime;
static bool debug = false;
void add_log_to_nc(const char* log, int len, uint32_t par_id){
  //printf("XXXXXXX: par_id: %d, len: %d\n", par_id, len);
  
  pxs_workers_g[par_id]->election_state_lock.lock(); // local lock;
  if(!pxs_workers_g[par_id]->is_leader){
    if(es->machine_id != 0)
	     Log_info("Did not find to be leader");
    pxs_workers_g[par_id]->election_state_lock.unlock();
    return;
  }
  pxs_workers_g[par_id]->election_state_lock.unlock();


  if(es->machine_id == 1 || es->machine_id == 2){
     if(debug)
	return;
    //Log_info("Submitting on behalf of new leader to worker");
    //return;
  }
  //debug = true;
  //len = 2;
  //printf("YYYYYYY:XXXXXXX: par_id: %d, len: %d\n", par_id, len);
  //if(submit_tot > 100000)return;
	//Log_info("add_log_to_nc: partition_id %d %d", len, par_id);
	//return;
	l_.lock();
	len = len;
	// submit_tot++;
	//endTime = std::chrono::high_resolution_clock::now();
	//auto paxos_entry = make_pair(log, make_pair(len, par_id));
	//submit_queue_nc.push(paxos_entry);
        //Log_info("Add log enters here");
	add_log_without_queue((char*)log, len, par_id);
	//Log_info("Add log exits here");
	l_.unlock();
}

void* PollSubQNc(void* arg){
   while(true){
     std::this_thread::sleep_for(std::chrono::milliseconds(100));
     l_.lock();
     //Log_info("Clearing queue of size %d", submit_queue_nc.size());
     int deleted = 0;
     while(!submit_queue_nc.empty()){
	     auto edt = std::chrono::high_resolution_clock::now();
	     auto x = submit_queue_nc.front();
	     //auto time_in_queue = std::chrono::duration_cast<std::chrono::nanoseconds>(edt - x.second.second).count();
	     //if(time_in_queue < 100.0)break;
	     submit_queue_nc.pop();
	     //free((char*)x.first);
	     //deleted++;
	     add_log_without_queue((char*)x.first, x.second.first, x.second.second);

     }
     //Log_info("Cleared %d entries", deleted);
     l_.unlock();
  }
   pthread_exit(nullptr);
   return nullptr;
}

shared_ptr<BulkPrepareLog> createBulkPrepare(int epoch, int machine_id){
  auto bulk_prepare = make_shared<BulkPrepareLog>();
  for(int i = 0; i < pxs_workers_g.size() - 1; i++){
    int32_t par_id = pxs_workers_g[i]->site_info_->partition_id_;
    slotid_t slot = pxs_workers_g[i]->n_current+1;
    bulk_prepare->min_prepared_slots.push_back(make_pair(par_id, slot));
  }
  bulk_prepare->epoch = epoch;
  bulk_prepare->leader_id = machine_id;
  return bulk_prepare;
}

shared_ptr<HeartBeatLog> createHeartBeat(int epoch, int machine_id){
  auto heart_beat = make_shared<HeartBeatLog>();
  heart_beat->epoch = epoch;
  heart_beat->leader_id = machine_id;
  return heart_beat;
}

shared_ptr<SyncLogRequest> createSyncLog(int epoch, int machine_id){
  auto syncLog = make_shared<SyncLogRequest>();
  syncLog->leader_id = machine_id;
  syncLog->epoch = epoch;
  for(int i = 0; i < pxs_workers_g.size() - 1; i++){
    auto ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    ps->mtx_.lock();
    slotid_t min_slot = ps->max_executed_slot_+1;
    ps->mtx_.unlock();
    syncLog->sync_commit_slot.push_back(min_slot);
  }
  return syncLog;
}

shared_ptr<SyncNoOpRequest> createSyncNoOpLog(int epoch, int machine_id){
  auto syncNoOpLog = make_shared<SyncNoOpRequest>();
  syncNoOpLog->leader_id = machine_id;
  syncNoOpLog->epoch = epoch;
  for(int i = 0; i < pxs_workers_g.size() - 1; i++){
    auto pw = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    pw->mtx_.lock();
    slotid_t min_slot = pw->max_executed_slot_+1;
    pw->mtx_.unlock();
    syncNoOpLog->sync_slots.push_back(min_slot);
  }
  return syncNoOpLog;
}


void send_no_ops_to_all_workers(int epoch){
  auto pw = pxs_workers_g.back();
  auto syncNoOpLog = createSyncNoOpLog(epoch, es->machine_id);
  auto ess = es;
  auto sp_job = std::make_shared<OneTimeJob>([pw, syncNoOpLog, ess](){
    int val = pw->SendSyncNoOpLog(syncNoOpLog);
    if(val == -1){
      ess->stuff_after_election_cond_.bcast();
    }
  });
  pxs_workers_g.back()->GetPollMgr()->add(sp_job);
  es->stuff_after_election_mutex_.lock();
  es->stuff_after_election_cond_.wait(es->stuff_after_election_mutex_);
  es->stuff_after_election_mutex_.unlock();
}

/*
change state to 1,
set epochs for workers
send synch rpc to followers.
*/

void send_sync_logs(int epoch){
  auto pw = pxs_workers_g.back();
  auto syncLog = createSyncLog(epoch, es->machine_id);
  auto ess = es;
  auto sp_job = std::make_shared<OneTimeJob>([pw, syncLog, ess](){
  int val = pw->SendSyncLog(syncLog);
  if(val == -1){
    ess->stuff_after_election_cond_.bcast();
  }
 });
 pxs_workers_g.back()->GetPollMgr()->add(sp_job);
 es->stuff_after_election_mutex_.lock();
 es->stuff_after_election_cond_.wait(es->stuff_after_election_mutex_);
 es->stuff_after_election_mutex_.unlock();
}

void sync_callbacks_for_new_leader(){
  for(int i = 0; i < pxs_workers_g.size(); i++){
    auto pw = pxs_workers_g[i];
    int partition_id_ = pw->site_info_->partition_id_;
    pw->register_apply_callback_par_id_return(leader_replay_cb[partition_id_]);
  }
}

void send_no_ops_for_mark(int epoch){
  string log = "no-ops:" + to_string(epoch);
  for(int i = 0; i < pxs_workers_g.size() - 1; i++){
    add_log_to_nc(log.c_str(), log.size(), i);
  }
}

void stuff_todo_leader_election(){
  es->state_lock();
  es->set_state(1);
  for(int i = 0; i < pxs_workers_g.size(); i++){
    pxs_workers_g[i]->election_state_lock.lock();
    pxs_workers_g[i]->cur_epoch = es->get_epoch();
    pxs_workers_g[i]->is_leader = 1;
    pxs_workers_g[i]->election_state_lock.unlock();
    auto ps = dynamic_cast<PaxosServer*>(pxs_workers_g[i]->rep_sched_);
    ps->mtx_.lock();
    //ps->cur_open_slot_ = max(ps->cur_open_slot_, ps->max_executed_slot_+1); // reset open slot counter
    //ps->cur_open_slot_ = ps->max_executed_slot_+1;
    ps->cur_open_slot_ = ps->max_committed_slot_+1;
    Log_info("The last committed slot %d and executed slot %d and open %d and touched %d", ps->max_committed_slot_, ps->max_executed_slot_, ps->cur_open_slot_, ps->max_touched_slot);
    ps->mtx_.unlock();
  }
  int epoch = es->get_epoch();
  es->state_unlock();
  sync_callbacks_for_new_leader();
  send_sync_logs(epoch);
  send_no_ops_to_all_workers(epoch);
  send_no_ops_for_mark(epoch);
}

void send_bulk_prep(int send_epoch){
  auto pw = pxs_workers_g.back();
  auto bp_log = createBulkPrepare(send_epoch, pw->site_info_->locale_id);
  auto ess = es;
  auto sp_job = std::make_shared<OneTimeJob>([&pw, bp_log, ess]() {
      int val = pw->SendBulkPrepare(bp_log);
      if(val != -1){
        ess->state_lock();
        ess->set_state(0);
        ess->set_epoch(val);
        ess->state_unlock();
      }
      ess->election_cond.bcast();
  });
  pxs_workers_g.back()->GetPollMgr()->add(sp_job);
}

// marker:ansh
void* electionMonitor(void* arg){
   // we need to take two situations into consideration: 1) startup; 2) exit
   // startup: sleep 5 seconds for the startup
   usleep(5 * 1000 * 1000);

   while(es->running){

    es->state_lock();
    /*if(es->machine_id == 2){ // marker:ansh for debug
      es->state_unlock();
      break;
    }*/
    if(es->cur_state == 1){
      if(es->did_not_send_prep()){
       //send_bulk_prep(es->get_epoch());
       //es->set_bulkprep_time();
      }
      es->state_unlock();
      es->sleep_timeout();
      continue;
    }
    if(!es->did_not_see_leader()){
      es->state_unlock();
      es->sleep_timeout();
      continue;
    }
    int send_epoch = es->set_epoch();
    es->state_unlock();
    send_bulk_prep(send_epoch);
    es->election_state.lock();
    es->election_cond.wait(es->election_state);
    es->election_state.unlock();
    es->state_lock();
    if(send_epoch != es->cur_epoch){
      es->state_unlock();
      continue;
    }
    es->state_unlock();
    stuff_todo_leader_election();
    leader_callback_();
  }
  pthread_exit(nullptr);
  return nullptr;
}

//marker:ansh
void* heartbeatMonitor(void* arg){
   while(es->running){
     es->sleep_heartbeat();
     es->state_lock();
     if(es->cur_state == 0){
      es->state_unlock();
      continue;
     }
     int send_epoch = es->get_epoch();
     es->state_unlock();
     auto pw = pxs_workers_g.back();
     auto hb_log = createHeartBeat(send_epoch, pw->site_info_->locale_id);
     auto ess = es;
     auto sp_job = std::make_shared<OneTimeJob>([pw, hb_log, ess]() {
        int val = pw->SendHeartBeat(hb_log);
        if(val != -1){
          ess->state_lock();
          ess->set_state(0);
          ess->set_epoch(val);
          ess->state_unlock();
        }
    });
    pxs_workers_g.back()->GetPollMgr()->add(sp_job);
  }
   pthread_exit(nullptr);
   return nullptr;
}



// to be called after setup 1; needed for multiprocess setup
int setup2(int action){  // action == 0 is default, action == 1 is forced to be follower
  auto server_infos = Config::GetConfig()->GetMyServers();
  if (server_infos.size() > 0) {
    server_launch_worker(server_infos);
  }
  if(action == 0 && es->machine_id == 0){
    es->set_state(1);
    es->set_epoch(2);
    es->set_leader(0);
    for(int i = 0; i < pxs_workers_g.size(); i++){
      pxs_workers_g[i]->is_leader = 1;
      pxs_workers_g[i]->cur_epoch = 2;
    }
  } else{
    es->set_state(0);
    es->set_epoch(0);
    es->set_leader(0);
  }
  Pthread_create(&submit_poll_th_, nullptr, PollSubQNc, nullptr);
  pthread_detach(submit_poll_th_);
  if (action != 1) {
      Pthread_create(&es->election_th_, nullptr, electionMonitor, nullptr);
      pthread_detach(es->election_th_);
  }
  Pthread_create(&es->heartbeat_th_, nullptr, heartbeatMonitor, nullptr);
  pthread_detach(es->heartbeat_th_);
  return 0;
}

void add_log(const char* log, int len, uint32_t par_id){
    auto startTime = std::chrono::high_resolution_clock::now();
    //read_log(log, len, "silo");
    for (auto& worker : pxs_workers_g) {
      if (!worker->IsLeader(par_id)) continue;
      worker->IncSubmit();
      break;
    }
    auto endTime = std::chrono::high_resolution_clock::now();
    auto paxos_entry = make_pair(log, make_pair(len, par_id));
    submit_queue.enqueue(paxos_entry);
    endTime = std::chrono::high_resolution_clock::now();
    submit_tot++;
    //add_time("enqueue_time",std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count(),1000.0*1000.0);
}


void worker_info_stats(size_t nthreads) {
    Log_info("# of paxos_workers is %d", pxs_workers_g.size());

    for (size_t par_id=0; par_id<nthreads; par_id++) {
      Log_info("par_id %d", par_id);
      size_t wIdx = 0;
      for (auto& worker : pxs_workers_g) {
          if (worker->IsLeader(par_id)) {
              Log_info("    work_index: %d, par_id: %d - IsLeader", wIdx, par_id);
          } else {
              Log_info("    work_index: %d, par_id: %d - Is not Leader", wIdx, par_id);
          };

          if (worker->IsPartition(par_id)) {
              Log_info("    work_index: %d, par_id: %d - IsPartition", wIdx, par_id);
          } else {
              Log_info("    work_index: %d, par_id: %d - Is not Partition", wIdx, par_id);
          };
          wIdx += 1 ;
      }
    }
}

void wait_for_submit(uint32_t par_id) {
    int total_submits = 0;
    //Log_info("The number of completed submits %ld", (int)submit_queue.size_approx());
 
    for (auto& worker : pxs_workers_g) {
        if(!worker->IsPartition(par_id))
          continue;
        worker->election_state_lock.lock();
        if (!worker->is_leader){
          worker->election_state_lock.unlock();
          continue;
        }
        worker->election_state_lock.unlock();
        //verify(worker->submit_pool != nullptr);
        //worker->submit_pool->wait_for_all();
	      Log_info("The number of completed submits n_current: %ld replay_queue: %ld par_id: %ld submit_tot: %ld", (int)worker->n_current, (int)worker->replay_queue.size_approx(), par_id, (int)worker->n_tot);
        worker->WaitForSubmit();
        total_submits = worker->n_tot;
    }
    for (auto& worker : pxs_workers_g) {
        if (!worker->IsPartition(par_id)) continue;
	      Log_info("Par_id %ld [partition], the number of completed submits %ld %ld", par_id, (int)worker->n_current, (int)worker->replay_queue.size_approx());
        worker->n_tot = total_submits;
        worker->WaitForSubmit();
    }
}
void pre_shutdown_step(){
    Log_info("shutdown Server Control Service after task finish total submit %d", (int)submit_tot);
    for (auto& worker : pxs_workers_g) {
        if (worker->hb_rpc_server_ != nullptr) {
            worker->scsi_->server_shutdown(nullptr);
        }
    }
}

void microbench_paxos_queue() {
    // register callback
    for (auto& worker : pxs_workers_g) {
        if (worker->IsLeader(worker->site_info_->partition_id_))
            worker->register_apply_callback([&worker](const char* log, int len) {
                Log_info("submit callback enter in");
                if (worker->submit_num >= worker->tot_num) return;
                worker->submit_num++;
                submit(log, len, worker->site_info_->partition_id_);
            });
        else
            worker->register_apply_callback([&worker](const char* log, int len) {
                std::ofstream outfile(string("log/") + string(worker->site_info_->name.c_str()) + string(".txt"), ios::app);
                outfile << log << std::endl;
                outfile.close();
            });
    }
    auto client_infos = Config::GetConfig()->GetMyClients();
    if (client_infos.size() <= 0) return;
    int concurrent = Config::GetConfig()->get_concurrent_txn();
    for (int i = 0; i < concurrent; i++) {
        message[i] = new char[len];
        message[i][0] = (i / 100) + '0';
        message[i][1] = ((i / 10) % 10) + '0';
        message[i][2] = (i % 10) + '0';
        for (int j = 3; j < len - 1; j++) {
            message[i][j] = (rand() % 10) + '0';
        }
        message[i][len - 1] = '\0';
    }
#ifdef CPU_PROFILE
    char prof_file[1024];
  Config::GetConfig()->GetProfilePath(prof_file);
  // start to profile
  ProfilerStart(prof_file);
#endif // ifdef CPU_PROFILE
    struct timeval t1, t2;
    gettimeofday(&t1, NULL);
    vector<std::thread> ths;
    int k = 0;
    for (int j = 0; j < 2; j++) {
        ths.push_back(std::thread([=, &k]() {
            int par_id = k++;
            for (int i = 0; i < concurrent; i++) {
                submit(message[i], len, par_id);
                // wait_for_submit(j);
            }
        }));
    }
    Log_info("waiting for submission threads.");
    for (auto& th : ths) {
        th.join();
    }
    while (1) {
        for (int j = 0; j < 2; j++) {
            wait_for_submit(j);
        }
        bool flag = true;
        for (auto& worker : pxs_workers_g) {
            if (worker->tot_num > worker->submit_num)
                flag = false;
        }
        if (flag) {
            Log_info("microbench finishes");
            break;
        }
    }
    gettimeofday(&t2, NULL);
    pxs_workers_g[0]->submit_tot_sec_ += t2.tv_sec - t1.tv_sec;
    pxs_workers_g[0]->submit_tot_usec_ += t2.tv_usec - t1.tv_usec;
#ifdef CPU_PROFILE
    // stop profiling
  ProfilerStop();
#endif // ifdef CPU_PROFILE

    Log_info("%s, time consumed: %f", pxs_workers_g[0]->site_info_->name.c_str(),
             pxs_workers_g[0]->submit_tot_sec_ + ((float)pxs_workers_g[0]->submit_tot_usec_) / 1000000);
    for (int i = 0; i < concurrent; i++) {
        delete message[i];
    }
    pre_shutdown_step();
}

// http://www.cse.cuhk.edu.hk/~ericlo/teaching/os/lab/9-PThread/Pass.html
struct args {
    int port;
    char* server_ip;
    int par_id;
};

static void
nc_pclock(char *msg, clockid_t cid)
{
    struct timespec ts;

    printf("%s", msg);
    if (clock_gettime(cid, &ts) == -1)
        std::cout << "clock_gettime error" << std::endl;
    printf("%4jd.%03ld\n", (intmax_t)ts.tv_sec, ts.tv_nsec / 1000000);
}

void *nc_start_server(void *input) {
    NetworkClientServiceImpl *impl = new NetworkClientServiceImpl();
    rrr::PollMgr *pm = new rrr::PollMgr();
    base::ThreadPool *tp = new base::ThreadPool();  // never use it
    rrr::Server *server = new rrr::Server(pm, tp);
    
    // We should count the child threads into consideration
    bool track_cputime=true;
    pthread_t *ps;
    if (track_cputime) ps = pm->GetPthreads(0);

    server->reg(impl);
    server->start((std::string(((struct args*)input)->server_ip)+std::string(":")+std::to_string(((struct args*)input)->port)).c_str()  );
    nc_services.push_back(std::shared_ptr<NetworkClientServiceImpl>(impl));
    int c=0;
    while (1) {
      c++;
      sleep(1);
      if (c==40) break;

      if (track_cputime) {
        clockid_t cid;
        int s = pthread_getcpuclockid(*ps, &cid);
        if (s != 0)
            std::cout << "error\n";
        nc_pclock("sub threads thread CPU time:   ", cid);
      }
      
      /*
      std::cout << "received on par_id: " << std::to_string(((struct args*)input)->par_id) << "\n";
      std::cout << "  new_order_counter:" << impl->counter_new_order << "\n"
                << "  counter_payement:" << impl->counter_payement << "\n"
                << "  counter_delivery:" << impl->counter_delivery << "\n"
                << "  counter_order_status:" << impl->counter_order_status << "\n"
                << "  counter_stock_level:" << impl->counter_stock_level << "\n"
                << "  in total:" << (impl->counter_new_order+impl->counter_payement+impl->counter_delivery+impl->counter_order_status+impl->counter_stock_level) << "\n\n" ;
                */
    }
}

// setup nthreads servers
void nc_setup_server(int nthreads, std::string host) {
  // std::map<std::string, std::string> hosts = getHosts(filename) ;
  // (char*)hosts["localhost"]
  for (int i=0; i<nthreads; i++) {
    struct args *ps = (struct args *)malloc(sizeof(struct args));
    ps->port=10010+i;
    ps->server_ip=(char*)host.c_str();
    ps->par_id=i;
    pthread_t ph_s;
    pthread_create(&ph_s, NULL, nc_start_server, (void *)ps);
    pthread_detach(ph_s);
    usleep(10 * 1000); // wait for 10ms
  }
}

std::vector<std::vector<int>> *nc_get_new_order_requests(int par_id) {
  return &nc_services[par_id]->new_order_requests;
}

std::vector<std::vector<int>>* nc_get_payment_requests(int par_id) {
  return &nc_services[par_id]->payment_requests;
}; 

std::vector<std::vector<int>>* nc_get_delivery_requests(int par_id) {
  return &nc_services[par_id]->delivery_requests;
}; 

std::vector<std::vector<int>>* nc_get_order_status_requests(int par_id) {
  return &nc_services[par_id]->order_status_requests;
}; 

std::vector<std::vector<int>>* nc_get_stock_level_requests(int par_id) {
  return &nc_services[par_id]->stock_level_requests;
}; 

std::vector<std::vector<int>>* nc_get_read_requests(int par_id) {
  return &nc_services[par_id]->read_requests;
}; 

std::vector<std::vector<int>>* nc_get_rmw_requests(int par_id) {
  return &nc_services[par_id]->rmw_requests;
}; 
