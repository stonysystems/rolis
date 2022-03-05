#include "paxos/server.h"
#include "paxos/commo.h"
#include "service.h"
#include "chrono"

namespace janus {
vector<shared_ptr<PaxosWorker>> pxs_workers_g = {};

moodycamel::ConcurrentQueue<shared_ptr<Coordinator>> PaxosWorker::coo_queue;
std::queue<shared_ptr<Coordinator>> PaxosWorker::coo_queue_nc;

shared_ptr<ElectionState> es_pw = ElectionState::instance();

static int volatile xx =
    MarshallDeputy::RegInitializer(MarshallDeputy::CONTAINER_CMD,
                                   []() -> Marshallable* {
                                     return new LogEntry;
                                   });
static int volatile xxx =
      MarshallDeputy::RegInitializer(MarshallDeputy::CMD_BLK_PXS,
                                     []() -> Marshallable* {
                                       return new BulkPaxosCmd;
                                     });
static int volatile x4 =
      MarshallDeputy::RegInitializer(MarshallDeputy::CMD_BLK_PREP_PXS,
                                     []() -> Marshallable* {
                                       return new BulkPrepareLog;
                                     });
static int volatile x5 =
      MarshallDeputy::RegInitializer(MarshallDeputy::CMD_HRTBT_PXS,
                                     []() -> Marshallable* {
                                       return new HeartBeatLog;
                                     });

static int volatile x6 =
      MarshallDeputy::RegInitializer(MarshallDeputy::CMD_SYNCREQ_PXS,
                                     []() -> Marshallable* {
                                       return new SyncLogRequest;
                                     });

static int volatile x7 =
      MarshallDeputy::RegInitializer(MarshallDeputy::CMD_SYNCRESP_PXS,
                                     []() -> Marshallable* {
                                       return new SyncLogResponse;
                                     });
static int volatile x8 =
      MarshallDeputy::RegInitializer(MarshallDeputy::CMD_SYNCNOOP_PXS,
                                     []() -> Marshallable* {
                                       return new SyncNoOpRequest;
                                     });
static int volatile x9 =
      MarshallDeputy::RegInitializer(MarshallDeputy::CMD_PREP_PXS,
                                     []() -> Marshallable* {
                                       return new PaxosPrepCmd;
                                     });

static int shared_ptr_apprch = 1;
Marshal& LogEntry::ToMarshal(Marshal& m) const {
  m << length;
  //Log_info("The legnth of the log is %d", length);
  if(shared_ptr_apprch){
	 if(operation_test.get())
	   m << std::string(operation_test.get(), length);
         else
	   m << log_entry;
  } else{
	  m << log_entry;
  }
  return m;
};

Marshal& LogEntry::FromMarshal(Marshal& m) {
  //return m;
  m >> length;
  if(false && shared_ptr_apprch){
	  std::string str;
	  m >> str;
	  // marker:ansh check here
	  //std::cout << str << " " << length << std::endl;
	  Log_info("FromMarshal %d", length);
	  operation_test = shared_ptr<char>(new char[length+1]);
	  operation_test.get()[length] = '\0';
	  memcpy(operation_test.get(), str.c_str(), str.length());
	  //fprintf(stderr, "%s\n", operation_test.get());
  }else{
 	 m >> log_entry;
  }
  return m;
};

void PaxosWorker::SetupBase() {
  auto config = Config::GetConfig();
  rep_frame_ = Frame::GetFrame(config->replica_proto_);
  rep_frame_->site_info_ = site_info_;
  rep_sched_ = rep_frame_->CreateScheduler();
  rep_sched_->loc_id_ = site_info_->locale_id;
  rep_sched_->partition_id_ = site_info_->partition_id_;
  this->tot_num = config->get_tot_req();
}

void PaxosWorker::Next(Marshallable& cmd) {
  //return;
  if (cmd.kind_ == MarshallDeputy::CONTAINER_CMD) {
    if (this->callback_par_id_return_ != nullptr) {
      //printf("a log is committed, par_id: %d\n", site_info_->partition_id_);
      auto& sp_log_entry = dynamic_cast<LogEntry&>(cmd);
      if(sp_log_entry.length == 0){
	 Log_info("Recieved a zero length log");
      }
      	if(true || !shared_ptr_apprch){
               if (sp_log_entry.length > 0) {
                  const char *log = sp_log_entry.log_entry.c_str() ;
                  //callback_par_id_(log, sp_log_entry.length, site_info_->partition_id_);
                  unsigned long long int r = callback_par_id_return_(log, sp_log_entry.length, site_info_->partition_id_, un_replay_logs_) ;
                  unsigned long long int latest_commit_id = r / 10;
                  // status: 1 => init, 2 => ending of paxos group, 3 => can't pass the safety check, 4 => complete replay
                  int status = r % 10;
		  //Log_info("status: %d\n", status);
                  if (status == 3) {
                      // we do a memory copy on log intentionally in case this log is freed by paxos
                      char *dest = (char *)malloc(sp_log_entry.length) ;
                      memcpy(dest, log, sp_log_entry.length) ;
                      un_replay_logs_.push(std::make_tuple(latest_commit_id, status, sp_log_entry.length, (const char*)dest)) ;
                  } else if (status == 1) {
                      std::cout << "this should never happen!!!" << std::endl;
                  }
               } else {
                 // the ending signal
                 const char *log = sp_log_entry.log_entry.c_str() ;
                 callback_par_id_return_(log, sp_log_entry.length, site_info_->partition_id_, un_replay_logs_) ;
               }
          } else {
              //std::cout << sp_log_entry.operation_test.get() << std::endl;
              //callback_par_id_(sp_log_entry.operation_test.get(), sp_log_entry.length, site_info_->partition_id_);
          }
      } else {
          verify(0);
      }
  } else {
    verify(0);
  }

  //n_current++;

  //Log_info("abc %d", site_info_->partition_id_);
  
  //if (n_current > n_tot) {
    //if(es_pw->machine_id == 0)
    //n_current++;
    //if(es_pw->machine_id == 0)
    //	Log_info("n_current increased here %d", (int)n_current);
    if(site_info_->locale_id == 0){
	   //if((int)n_current%100 == 0)Log_info("current commits are progressing, current %d", (int)n_current);
    }
    if (n_current >= n_tot) {
      //Log_info("Current pair id %d loc id %d n_current and n_tot and accept size is %d %d", site_info_->partition_id_, site_info_->locale_id, (int)n_current, (int)n_tot);
      finish_cond.bcast();
    }
  //}
}

void PaxosWorker::SetupService() {
  std::string bind_addr = site_info_->GetBindAddress();
  int n_io_threads = 1;
  svr_poll_mgr_ = new rrr::PollMgr(n_io_threads);
  if (rep_frame_ != nullptr) {
    services_ = rep_frame_->CreateRpcServices(site_info_->id,
                                              rep_sched_,
                                              svr_poll_mgr_,
                                              scsi_);
  }
  uint32_t num_threads = 1;
  thread_pool_g = new base::ThreadPool(num_threads);

  // init rrr::Server
  rpc_server_ = new rrr::Server(svr_poll_mgr_, thread_pool_g);

  // reg services
  for (auto service : services_) {
    rpc_server_->reg(service);
  }

  // start rpc server
  Log_debug("starting server at %s", bind_addr.c_str());
  std::cout << "starting server at " << bind_addr.c_str() << std::endl;
  int ret = rpc_server_->start(bind_addr.c_str());
  if (ret != 0) {
    Log_fatal("server launch failed.");
    std::cout << "server launch failed.\n";
  }

  Log_info("Server %s ready at %s",
           site_info_->name.c_str(),
           bind_addr.c_str());
}

void PaxosWorker::SetupCommo() {
  if (rep_frame_) {
    rep_commo_ = rep_frame_->CreateCommo(svr_poll_mgr_);
    if (rep_commo_) {
      rep_commo_->loc_id_ = site_info_->locale_id;
    }
    rep_sched_->commo_ = rep_commo_;
  }
  //if (IsLeader(site_info_->partition_id_))submit_pool = new SubmitPool();
}

void PaxosWorker::SetupHeartbeat() {
  bool hb = Config::GetConfig()->do_heart_beat();
  if (!hb) return;
  auto timeout = Config::GetConfig()->get_ctrl_timeout();
  scsi_ = new ServerControlServiceImpl(timeout);
  int n_io_threads = 1;
  svr_hb_poll_mgr_g = new rrr::PollMgr(n_io_threads);
  hb_thread_pool_g = new rrr::ThreadPool(1);
  hb_rpc_server_ = new rrr::Server(svr_hb_poll_mgr_g, hb_thread_pool_g);
  hb_rpc_server_->reg(scsi_);

  auto port = site_info_->port + CtrlPortDelta;
  std::string addr_port = std::string("0.0.0.0:") +
                          std::to_string(port);
  hb_rpc_server_->start(addr_port.c_str());
  if (hb_rpc_server_ != nullptr) {
    // Log_info("notify ready to control script for %s", bind_addr.c_str());
    scsi_->set_ready();
  }
  Log_info("heartbeat setup for %s on %s",
           site_info_->name.c_str(), addr_port.c_str());
}

void PaxosWorker::WaitForShutdown() {
  if (submit_pool != nullptr) {
    delete submit_pool;
    submit_pool = nullptr;
  }
  if (hb_rpc_server_ != nullptr) {
//    scsi_->server_heart_beat();
    scsi_->wait_for_shutdown();
    delete hb_rpc_server_;
    delete scsi_;
    svr_hb_poll_mgr_g->release();
    hb_thread_pool_g->release();

    for (auto service : services_) {
      if (DepTranServiceImpl* s = dynamic_cast<DepTranServiceImpl*>(service)) {
        auto& recorder = s->recorder_;
        if (recorder) {
          auto n_flush_avg_ = recorder->stat_cnt_.peek().avg_;
          auto sz_flush_avg_ = recorder->stat_sz_.peek().avg_;
          Log::info("Log to disk, average log per flush: %lld,"
                    " average size per flush: %lld",
                    n_flush_avg_, sz_flush_avg_);
        }
      }
    }
  }
}

void PaxosWorker::ShutDown() {
  Log_info("site %s deleting services, num: %d %d %d %d", site_info_->name.c_str(), services_.size(), 0, (int)n_current, (int)n_tot);
  verify(rpc_server_ != nullptr);
  delete rpc_server_;
  rpc_server_ = nullptr;
  for (auto service : services_) {
    delete service;
  }
  thread_pool_g->release();
  for (auto c : created_coordinators_) {
    delete c;
  }
  if (rep_sched_ != nullptr) {
    delete rep_sched_;
  }
}

void PaxosWorker::IncSubmit(){	
	n_tot++;
}

void PaxosWorker::BulkSubmit(const vector<shared_ptr<Coordinator>>& entries){
    //Log_info("Obtaining bulk submit of size %d through coro", (int)entries.size());
    //Log_debug("Current n_submit and n_current is %d %d", (int)n_submit, (int)n_current);
    //marker:ansh use per thread stuff for optimization
    auto sp_cmd = make_shared<BulkPaxosCmd>();
    election_state_lock.lock();
    ballot_t send_epoch = this->cur_epoch;
    election_state_lock.unlock();
    sp_cmd->leader_id = es_pw->machine_id;
    //Log_debug("Current reference count before submit : %d", sp_cmd.use_count());
    for(auto coo : entries){
        auto mpc = dynamic_pointer_cast<CoordinatorMultiPaxos>(coo);
        sp_cmd->slots.push_back(mpc.get()->slot_id_);
        sp_cmd->ballots.push_back(send_epoch);
        verify(mpc->cmd_ != nullptr);
        //auto x = dynamic_pointer_cast<LogEntry>(mpc->cmd_);
        //read_log(x.get()->operation_test.get(), x.get()->length, "BulkSubmit");
        MarshallDeputy* md =  new MarshallDeputy(mpc.get()->cmd_);
        sp_cmd->cmds.push_back(shared_ptr<MarshallDeputy>(md));
        //auto x = dynamic_pointer_cast<LogEntry>(md->sp_data_);
       // read_log(x.get()->operation_test.get(), x.get()->length, "BulkSubmit");
    }
    auto sp_m = dynamic_pointer_cast<Marshallable>(sp_cmd);
    //return;
    //return;
    //n_current += (int)entries.size();
    //n_submit -= (int)entries.size();
    //Log_info("Current pair id %d n_current and n_tot is %d %d", site_info_->partition_id_, (int)n_current, (int)n_tot);
    _BulkSubmit(sp_m, entries.size());
    Log_debug("Current reference count after submit: %d", sp_cmd.use_count());
}

inline void PaxosWorker::_BulkSubmit(shared_ptr<Marshallable> sp_m, int cnt = 0){
    auto coord = shared_ptr<Coordinator>(rep_frame_->CreateBulkCoordinator(Config::GetConfig(), 0));
    coord.get()->par_id_ = site_info_->partition_id_;
    coord.get()->loc_id_ = site_info_->locale_id;
    //if(es_pw->machine_id == 1 || es_pw->machine_id == 2)
	//Log_info("Submitting on behalf of new leader");
    coord.get()->BulkSubmit(sp_m, [this, cnt]() {
      this->n_current += cnt;
      //if((int)n_current%2 == 0)Log_info("current commits are progressing, current %d", (int)n_current);
      if(this->n_current >= this->n_tot)this->finish_cond.bcast();
    });
}

// marker:ansh
int PaxosWorker::SendBulkPrepare(shared_ptr<BulkPrepareLog> bp_log){
    auto sp_m = dynamic_pointer_cast<Marshallable>(bp_log);
  ballot_t received_epoch = -1;
  auto coord = rep_frame_->CreateBulkCoordinator(Config::GetConfig(), 0);
  coord->par_id_ = site_info_->partition_id_;
  coord->loc_id_ = site_info_->locale_id;
  auto sp_quorum = coord->commo_->BroadcastBulkPrepare(site_info_->partition_id_, sp_m, [&received_epoch](ballot_t ballot, int valid) {
    Log_info("BulkPrepare: response received %d", valid);
    if(!valid){
      //Log_info("BulkPrepare: response received");
      received_epoch = max(received_epoch, ballot);
    }
  });
  Log_info("BulkPrepare: waiting for response");
  sp_quorum->Wait();
  if (sp_quorum->Yes()) {
    Log_info("SendBulkPrepare: Leader election successfull");
    return -1;
  } else{
    Log_debug("SendBulkPrepare: Leader election unsuccessfull");
  }
  return received_epoch;
}

// marker:ansh
int PaxosWorker::SendHeartBeat(shared_ptr<HeartBeatLog> hb_log){
  auto sp_m = dynamic_pointer_cast<Marshallable>(hb_log);
  ballot_t received_epoch = -1;
  auto coord = rep_frame_->CreateBulkCoordinator(Config::GetConfig(), 0);
  coord->par_id_ = site_info_->partition_id_;
  coord->loc_id_ = site_info_->locale_id;
  auto sp_quorum = coord->commo_->BroadcastHeartBeat(site_info_->partition_id_, sp_m, [&received_epoch](ballot_t ballot, int resp_type) {
    if(!resp_type)
      received_epoch = ballot;
  });
  sp_quorum->Wait();
  if (sp_quorum->Yes()) {
    return -1;
  }
  return received_epoch;
}

int PaxosWorker::SendSyncLog(shared_ptr<SyncLogRequest> sync_log_req){  // what's we need
  auto sp_m = dynamic_pointer_cast<Marshallable>(sync_log_req);
  ballot_t received_epoch = -1;
  auto coord = rep_frame_->CreateBulkCoordinator(Config::GetConfig(), 0);
  coord->par_id_ = site_info_->partition_id_;
  coord->loc_id_ = site_info_->locale_id;
  bool done = false;
  auto es_pww = es_pw;
  vector<shared_ptr<SyncLogResponse>> responses;
  auto sp_quorum = coord->commo_->BroadcastSyncLog(site_info_->partition_id_, 
                                                   sp_m, 
                                                   [&received_epoch, &done, es_pww, &responses](shared_ptr<MarshallDeputy> md, 
                                                                                    ballot_t ballot, 
                                                                                    int resp_type) {
    if(!resp_type)
      es_pww->step_down(ballot);
    else{
      if(!done){
        responses.push_back(dynamic_pointer_cast<SyncLogResponse>(md->sp_data_));
      } else{
        return;
      }
    }
  });
  sp_quorum->Wait();
  done = true;
  if (sp_quorum->Yes()) {
    map<pair<int,slotid_t>, shared_ptr<MarshallDeputy>> commited_slots;
    for(int i = 0; i < responses.size(); i++){
      for(int j = 0; j < responses[i]->sync_data.size(); j++){
        auto bp_cmd = dynamic_pointer_cast<BulkPaxosCmd>(responses[i]->sync_data[j]->sp_data_);
        for(int k = 0; k < bp_cmd->slots.size(); k++){
          commited_slots[make_pair(j, bp_cmd->slots[k])] = bp_cmd->cmds[k];
        }
      }
    }
    Log_info("Responses size is %d", responses.size());
    for(int i = 0; i < responses.size(); i++){
      for(int j = 0; j < responses[i]->missing_slots.size(); j++){
        auto ps_j = dynamic_cast<PaxosServer*>(pxs_workers_g[j]->rep_sched_);
        for(int k = 0; k < responses[i]->missing_slots[j].size(); k++){
          auto inst = ps_j->GetInstance(responses[i]->missing_slots[j][k]);
          if(inst->committed_cmd_){
	    //Log_info("The slots are for partition %d slot %d", j, responses[i]->missing_slots[j][k]);
            auto tmp = inst->committed_cmd_;
            commited_slots[make_pair(j, responses[i]->missing_slots[j][k])] = make_shared<MarshallDeputy>(MarshallDeputy(tmp));
          }
        }
      }
    }

    vector<shared_ptr<BulkPaxosCmd>> sync_cmds;
    for(int i = 0; i < pxs_workers_g.size() - 1; i++){
      auto bp_cmd = make_shared<BulkPaxosCmd>();
      bp_cmd->leader_id = es_pw->machine_id;
      sync_cmds.push_back(bp_cmd);
    }
    for(auto const& x : commited_slots){
      sync_cmds[x.first.first]->slots.push_back(x.first.second);
      sync_cmds[x.first.first]->cmds.push_back(x.second);
      sync_cmds[x.first.first]->ballots.push_back(sync_log_req->epoch);
    }
    vector<shared_ptr<PaxosAcceptQuorumEvent>> events;
    for(int i = 0; i < pxs_workers_g.size() - 1; i++){
      if(sync_cmds[i]->ballots.size() == 0)
        continue;
      //Log_info("Should receive some uncommitted slots here %d", i);
      //for(int kk = 0; kk < sync_cmds[i]->slots.size(); kk++)
      //      std::cout << sync_cmds[i]->slots[kk] << " ";
      //std::cout << std::endl;
      auto pw = pxs_workers_g[i];
      auto send_cmd = dynamic_pointer_cast<Marshallable>(sync_cmds[i]);
      auto sp_quorum = pw->rep_commo_->BroadcastSyncCommit(i, 
                                                           send_cmd,
                                                           [es_pww](ballot_t ballot, int valid){
          if(!valid){
            es_pww->step_down(ballot);
          }
      });
      events.push_back(sp_quorum);
      //sp_quorum->Wait();
    }
    for(int i = 0; i < events.size(); i++){
      events[i]->Wait();
    }
    return -1;
  }
  return received_epoch;
}

int PaxosWorker::SendSyncNoOpLog(shared_ptr<SyncNoOpRequest> sync_log_req){
  auto sp_m = dynamic_pointer_cast<Marshallable>(sync_log_req);
  ballot_t received_epoch = -1;
  auto coord = rep_frame_->CreateBulkCoordinator(Config::GetConfig(), 0);
  coord->par_id_ = site_info_->partition_id_;
  coord->loc_id_ = site_info_->locale_id;
  bool done = false;
  auto es_pww = es_pw;
  auto sp_quorum = coord->commo_->BroadcastSyncNoOps(site_info_->partition_id_, 
                                                   sp_m, 
                                                   [&received_epoch, &done, es_pww](ballot_t ballot, 
                                                                                    int resp_type) {
    if(!resp_type)
      es_pww->step_down(ballot);
    else{
      if(!done){
      } else{
        return;
      }
    }
  });
  sp_quorum->Wait();
  done = true;
  if(sp_quorum->Yes()){
    return -1;
  }
  return received_epoch;
}

void PaxosWorker::AddAccept(shared_ptr<Coordinator> coord) {
  //Log_info("current batch cnt %d", cnt);
  PaxosWorker::coo_queue.enqueue(coord);
}

int PaxosWorker::deq_from_coo(vector<shared_ptr<Coordinator>>& current){
  int qcnt = PaxosWorker::coo_queue.try_dequeue_bulk(&current[0], cnt);
  return qcnt;
}


void* PaxosWorker::StartReadAccept(void* arg){
  PaxosWorker* pw = (PaxosWorker*)arg;
  //std::vector<shared_ptr<Coordinator>> current(pw->cnt, nullptr);
  int sent = 0;
  while (!pw->stop_flag) {
    std::vector<shared_ptr<Coordinator>> current(pw->cnt, nullptr);
    int cnt = pw->deq_from_coo(current);
    if(cnt <= 0)continue;
    std::vector<shared_ptr<Coordinator>> sub(current.begin(), current.begin() + cnt);
    //Log_debug("Pushing coordinators for bulk accept coordinators here having size %d %d %d %d", (int)sub.size(), pw->n_current.load(), pw->n_tot.load(),pw->site_info_->locale_id);
    auto sp_job = std::make_shared<OneTimeJob>([&pw, sub]() {
      pw->BulkSubmit(sub);
    });
    pw->GetPollMgr()->add(sp_job);
    sent += cnt;
    if(sent % 2 == 0)Log_info("Total submits %d", sent);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  pthread_exit(nullptr);
  return nullptr;
}

void PaxosWorker::AddAcceptNc(shared_ptr<Coordinator> coord) {
  //nc_submit_l_.lock();
  //PaxosWorker::coo_queue_nc.push(coord);
  //nc_submit_l_.unlock();
  all_coords[bulk_writer++] = coord;
}

void PaxosWorker::submitJob(std::shared_ptr<Job> sp_job){
	GetPollMgr()->add(sp_job);
}

void* PaxosWorker::StartReadAcceptNc(void* arg){
  PaxosWorker* pw = (PaxosWorker*)arg;
  std::vector<shared_ptr<Coordinator>> current(pw->cnt, nullptr);
  int sent = 0;
  while (!pw->stop_flag) {
    int cur_req = pw->cnt;
    /*pw->nc_submit_l_.lock();
    while(!PaxosWorker::coo_queue_nc.empty() && cur_req > 0){
      auto x = PaxosWorker::coo_queue_nc.front();
      PaxosWorker::coo_queue_nc.pop();
      current.push_back(x);
      cur_req--;
    }
    pw->nc_submit_l_.unlock();*/
    while(cur_req > 0 and pw->all_coords[pw->bulk_reader] != nullptr){
	   //pw->bulk_reader++; 
	   current[pw->cnt - cur_req] = pw->all_coords[pw->bulk_reader];
	   cur_req--;
	   pw->bulk_reader++;
    }
    int cnt = pw->cnt - cur_req;
    if(cnt == 0)continue;
    std::vector<shared_ptr<Coordinator>> curr2(current.begin(), current.begin() + cnt);
    //Log_info("Pushing coordinators for bulk accept coordinators here having size %d %d %d %d", (int)curr2.size(), pw->n_current.load(), pw->n_tot.load(),pw->site_info_->locale_id);
    auto sp_job = std::make_shared<OneTimeJob>([&pw, curr2]() {
      pw->BulkSubmit(curr2);
    });
    /*Log_info("alalslal %d %d %d", cnt, (int)pw->n_tot, (int)pw->n_current);
    if(pw->n_current + cnt >= pw->n_tot){
	    pw->finish_cond.bcast();
    }*/
    auto strt = std::chrono::high_resolution_clock::now();
    pw->submitJob(sp_job);
    auto endt = std::chrono::high_resolution_clock::now();
    sent += cnt;
    //if(sent % 2 == 0)Log_info("The number of submitted entries is %d %d", sent, cnt);
    //pw->n_current+= cnt;
    auto secs = std::chrono::duration_cast<std::chrono::nanoseconds>(endt - strt).count();
    //if(sent % 2 == 0)Log_info("Time spent is submitting the job %f", secs/(1000.0*1000.0*1000.0));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  pthread_exit(nullptr);
  return nullptr;
}

void PaxosWorker::WaitForSubmit() {
  /*while(true){
	sleep(1);
        Log_info("wait for task, amount: %d - n_tot: %d, n_current: %d", (int)n_tot-(int)n_current, (int)n_tot, (int)n_current);
  }*/
  while (n_current < n_tot) {
    finish_mutex.lock();
    Log_info("wait for task, amount: %d - n_tot: %d, n_current: %d", (int)n_tot-(int)n_current, (int)n_tot, (int)n_current);
    finish_cond.wait(finish_mutex);
    //Log_info("wait for task, amount: %d - n_tot: %d, n_current: %d", (int)n_tot-(int)n_current, (int)n_tot, (int)n_current);
    finish_mutex.unlock();
  }
  Log_debug("finish task.");
}

void PaxosWorker::InitQueueRead(){
  if(IsLeader(site_info_->partition_id_)){
    stop_flag = false;
    //Pthread_create(&bulkops_th_, nullptr, PaxosWorker::StartReadAcceptNc, this);
    //pthread_detach(bulkops_th_);
  }
}

void PaxosWorker::AddReplayEntry(Marshallable& entry){
  Marshallable *p = &entry;
  replay_queue.enqueue(p);
}

void* PaxosWorker::StartReplayRead(void* arg){
  PaxosWorker* pw = (PaxosWorker*)arg;
  while(!pw->stop_replay_flag){
    Marshallable* p;
    auto res = pw->replay_queue.try_dequeue(p);
    if(!res)continue;
    pw->Next(*p);
  }
}

PaxosWorker::PaxosWorker() {
  stop_replay_flag = true;
  Pthread_create(&replay_th_, nullptr, PaxosWorker::StartReplayRead, this);
  pthread_detach(replay_th_);
}

PaxosWorker::~PaxosWorker() {
  Log_debug("Ending worker with n_tot %d and n_current %d", (int)n_tot, (int)n_current);
  stop_flag = true;
  stop_replay_flag = true;
}


void PaxosWorker::Submit(const char* log_entry, int length, uint32_t par_id) {
  //Log_info("Entering PaxosWorker::Submit  here\n");
  //if (!IsLeader(par_id)) return;
  //read_log(log_entry, length, "silo");
  auto sp_cmd = make_shared<LogEntry>();
  if(!shared_ptr_apprch){
	  sp_cmd->log_entry = string(log_entry,length);
  }else{
    //sp_cmd->operation_ = (char*)string(log_entry,length).c_str();
    // sp_cmd->operation_test = shared_ptr<char>((char*)string(log_entry,length).c_str());
	  sp_cmd->operation_test = shared_ptr<char>((char*)malloc(length));
    memcpy(sp_cmd->operation_test.get(), log_entry, length);
  }
  //Log_info("PaxosWorker::Submit Log=%s",operation_);
  sp_cmd->length = length;
  auto sp_m = dynamic_pointer_cast<Marshallable>(sp_cmd);
  _Submit(sp_m);
  //free((char*)log_entry);
}

inline void PaxosWorker::_Submit(shared_ptr<Marshallable> sp_m) {
  //mtx_worker_submit.lock();	
  // finish_mutex.lock();
  //n_current++;
  //n_submit--;
  //n_tot++;
  // finish_mutex.unlock();
  static cooid_t cid{1};
  static id_t id{1};
  verify(rep_frame_ != nullptr);
  auto coord = rep_frame_->CreateCoordinator(cid++,
                                                     Config::GetConfig(),
                                                     0,
                                                     nullptr,
                                                     id++,
                                                     nullptr);
  //mtx_worker_submit.unlock();
  coord->par_id_ = site_info_->partition_id_;
  coord->loc_id_ = site_info_->locale_id;
  //marker:ansh slot_hint not being used anymore.
  slotid_t x = ((PaxosServer*)rep_sched_)->get_open_slot();
  coord->set_slot(x);
  //created_coordinators_.push_back(coord);
  //coord->cmd_ = sp_m;
  coord->assignCmd(sp_m);
  Log_debug("PaxosWorker: job submitted for slot %d", x);
  if(stop_flag != true) {
    auto sp_coo = shared_ptr<Coordinator>(coord);
    //created_coordinators_shrd.push_back(sp_coo);
    //n_current++;
    vector<shared_ptr<Coordinator>> curr2;
    curr2.push_back(sp_coo);
    //PaxosWorker* pw = this;
    //Log_info("PaxosWorker: job submitted for slot %d", x);
    auto sp_job = std::make_shared<OneTimeJob>([this, curr2]() {
      this->BulkSubmit(curr2);
    });
    submitJob(sp_job);
    //BulkSubmit(vector<shared_ptr<Coordinator>>{sp_coo});
    //AddAcceptNc(sp_coo);
  } else{
    coord->Submit(sp_m);
  }
}

bool PaxosWorker::IsLeader(uint32_t par_id) {
  verify(rep_frame_ != nullptr);
  verify(rep_frame_->site_info_ != nullptr);
  return rep_frame_->site_info_->partition_id_ == par_id &&
         rep_frame_->site_info_->locale_id == 0;
}

bool PaxosWorker::IsPartition(uint32_t par_id) {
  verify(rep_frame_ != nullptr);
  verify(rep_frame_->site_info_ != nullptr);
  return rep_frame_->site_info_->partition_id_ == par_id;
}

void PaxosWorker::register_apply_callback(std::function<void(const char*, int)> cb) {
  this->callback_ = cb;
  verify(rep_sched_ != nullptr);
  rep_sched_->RegLearnerAction(std::bind(&PaxosWorker::Next,
                                         this,
                                         std::placeholders::_1));
}

void PaxosWorker::register_apply_callback_par_id(std::function<void(const char *&, int, int)> cb) {
    this->callback_par_id_ = cb;
    verify(rep_sched_ != nullptr);
    rep_sched_->RegLearnerAction(std::bind(&PaxosWorker::Next,
                                           this,
                                           std::placeholders::_1));
}

    void PaxosWorker::register_apply_callback_par_id_return(std::function<unsigned long long int(const char *&, int, int, std::queue<std::tuple<unsigned long long int, int, int, const char *>> &)> cb) {
        this->callback_par_id_return_ = cb;
        verify(rep_sched_ != nullptr);
        rep_sched_->RegLearnerAction(std::bind(&PaxosWorker::Next,
                                               this,
                                               std::placeholders::_1));
    }

} // namespace janus
