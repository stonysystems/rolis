#pragma once

#include "__dep__.h"
#include "coordinator.h"
#include "benchmark_control_rpc.h"
#include "frame.h"
#include "scheduler.h"
#include "communicator.h"
#include "config.h"
#include "./paxos/coordinator.h"
#include "concurrentqueue.h"

namespace janus {

	typedef std::chrono::time_point<std::chrono::high_resolution_clock> timepoint;

  class BulkPaxosCmd;

	inline void read_log(const char* log, int length, const char* custom){
		unsigned long long int cid = 0;
		memcpy(&cid, log, sizeof(unsigned long long int));
		Log_info("commit id %lld and length %d from %s", cid, length, custom);
	}

	inline size_t track_write(int fd, const void* p, size_t len, int offset){
		const char* x = (const char*)p;
		ssize_t sz = ::write(fd, x + offset, len - offset);
		if(sz > len - offset || sz <= 0){
			//std::cout << "gahdamn " <<  sz << std::endl;
			//Log_info("Gahdamn, speed it %lld", sz);
			return 0;
		}
		return sz;
	}

	class SubmitPool {
		private:
			struct start_submit_pool_args {
				SubmitPool* subpool;
			};

			int n_;
			std::list<std::function<void()>*>* q_;
			pthread_cond_t not_empty_;
			pthread_mutex_t m_;
			pthread_mutex_t run_;
			pthread_t th_;
			bool should_stop_{false};

			static void* start_thread_pool(void* args) {
				start_submit_pool_args* t_args = (start_submit_pool_args *) args;
				t_args->subpool->run_thread();
				delete t_args;
				pthread_exit(nullptr);
				return nullptr;
			}
			void run_thread() {
				for (;;) {
					function<void()>* job = nullptr;
					Pthread_mutex_lock(&m_);
					while (q_->empty()) {
						Pthread_cond_wait(&not_empty_, &m_);
					}
					Pthread_mutex_lock(&run_);
					job = q_->front();
					q_->pop_front();
					Pthread_mutex_unlock(&m_);
					if (job == nullptr) {
						Pthread_mutex_unlock(&run_);
						break;
					}
					(*job)();
					delete job;
					Pthread_mutex_unlock(&run_);
				}
			}
			bool try_pop(std::function<void()>** t) {
				bool ret = false;
				if (!q_->empty()) {
					ret = true;
					*t = q_->front();
					q_->pop_front();
				}
				return ret;
			}

		public:
			SubmitPool()
				: n_(1), th_(0), q_(new std::list<std::function<void()>*>), not_empty_(), m_(), run_() {
					verify(n_ >= 0);
					Pthread_mutex_init(&m_, nullptr);
					Pthread_mutex_init(&run_, nullptr);
					Pthread_cond_init(&not_empty_, nullptr);
					for (int i = 0; i < n_; i++) {
						start_submit_pool_args* args = new start_submit_pool_args();
						args->subpool = this;
						Pthread_create(&th_, nullptr, SubmitPool::start_thread_pool, args);
					}
				}
			SubmitPool(const SubmitPool&) = delete;
  SubmitPool& operator=(const SubmitPool&) = delete;
  ~SubmitPool() {
    should_stop_ = true;
    for (int i = 0; i < n_; i++) {
      Pthread_mutex_lock(&m_);
      q_->push_back(nullptr); //death pill
      Pthread_cond_signal(&not_empty_);
      Pthread_mutex_unlock(&m_);
    }
    for (int i = 0; i < n_; i++) {
      Pthread_join(th_, nullptr);
    }
    Log_debug("%s: enter in wait_for_all", __FUNCTION__);
    wait_for_all();
    Pthread_cond_destroy(&not_empty_);
    Pthread_mutex_destroy(&m_);
    Pthread_mutex_destroy(&run_);
    delete q_;
  }
  void wait_for_all() {
    for (int i = 0; i < n_; i++) {
      function<void()>* job;
      Pthread_mutex_lock(&m_);
      Pthread_mutex_lock(&run_);
      while (try_pop(&job)) {
        if (job != nullptr) {
          (*job)();
          delete job;
        }
      }
      Pthread_mutex_unlock(&m_);
      Pthread_mutex_unlock(&run_);
    }
  }
  int add(const std::function<void()>& f) {
    if (should_stop_) {
      return -1;
    }
    Pthread_mutex_lock(&m_);
    q_->push_back(new function<void()>(f));
    Pthread_cond_signal(&not_empty_);
    Pthread_mutex_unlock(&m_);
    return 0;
  }
};

class BulkPrepareLog : public Marshallable {
  public:
  vector<pair<uint32_t,slotid_t>> min_prepared_slots;
  uint32_t leader_id;
  int epoch;

  BulkPrepareLog(): Marshallable(MarshallDeputy::CMD_BLK_PREP_PXS){

  }

  Marshal& ToMarshal(Marshal& m) const override {
      m << (int32_t) min_prepared_slots.size();
      for(auto i : min_prepared_slots){
          m << i;
      }
      m << leader_id;
      m << epoch;
      return m;
  }

  Marshal& FromMarshal(Marshal& m) override {
    int32_t sz;
    m >> sz;
    for(int i = 0; i < sz; i++){
      pair<uint32_t,slotid_t> pr;
      m >> pr;
      min_prepared_slots.push_back(pr);
    }
    m >> leader_id;
    m >> epoch;
    return m;
  }

};

class PaxosPrepCmd : public Marshallable {
  public:
  vector<slotid_t> slots{};
  vector<ballot_t> ballots{};
  int leader_id;

  PaxosPrepCmd(): Marshallable(MarshallDeputy::CMD_PREP_PXS){

  }

  Marshal& ToMarshal(Marshal& m) const override {
      m << (int32_t) slots.size();
      for(auto i : slots){
          m << i;
      }
      m << (int32_t) slots.size();
      for(auto i : ballots){
          m << i;
      }
      m << leader_id;
      return m;
  }

  Marshal& FromMarshal(Marshal& m) override {
    int32_t sz;
    m >> sz;
    for(int i = 0; i < sz; i++){
      slotid_t x;
      m >> x;
      slots.push_back(x);
    }
    m >> sz;
    for(int i = 0; i < sz; i++){
      ballot_t x;
      m >> x;
      ballots.push_back(x);
    }
    m >> leader_id;
    return m;
  }

};

class HeartBeatLog : public Marshallable {
  public:
  uint32_t leader_id;
  int epoch;

  HeartBeatLog(): Marshallable(MarshallDeputy::CMD_HRTBT_PXS){

  }

  Marshal& ToMarshal(Marshal& m) const override {
      m << leader_id;
      m << epoch;
      return m;
  }

  Marshal& FromMarshal(Marshal& m) override {
    m >> leader_id;
    m >> epoch;
    return m;
  }

};

class SyncLogRequest : public Marshallable {
  public:
    int leader_id;
    ballot_t epoch;
    vector<slotid_t> sync_commit_slot;
    SyncLogRequest(): Marshallable(MarshallDeputy::CMD_SYNCREQ_PXS){

    }

    Marshal& ToMarshal(Marshal& m) const override {
      m << leader_id;
      m << epoch;
      m << (int32_t)sync_commit_slot.size();
      for(int i = 0; i < sync_commit_slot.size(); i++){
        m << sync_commit_slot[i];
      }
      return m;
    }

    Marshal& FromMarshal(Marshal& m) override {
      m >> leader_id;
      m >> epoch;
      int32_t sz;
      m >> sz;
      for(int i = 0; i < sz; i++){
        slotid_t x;
        m >> x;
        sync_commit_slot.push_back(x);
      }
      return m;
    }
};

class SyncLogResponse : public Marshallable {
  public:
    vector<shared_ptr<MarshallDeputy>> sync_data;
    vector<vector<slotid_t>> missing_slots;
    SyncLogResponse(): Marshallable(MarshallDeputy::CMD_SYNCRESP_PXS){

    }

    Marshal& ToMarshal(Marshal& m) const override {
      m << (int32_t)sync_data.size();
      for(int i = 0; i < sync_data.size(); i++){
        m << *sync_data[i];
      }
      m << (int32_t)missing_slots.size();
      for(int i = 0; i < missing_slots.size(); i++){
        m << (int32_t)missing_slots[i].size();
        for(int j = 0; j < missing_slots[i].size(); j++){
          m << missing_slots[i][j];
        }
      }
    }

    Marshal& FromMarshal(Marshal& m) override {
      int32_t sz;
      m >> sz;
      for(int i = 0; i < sz; i++){
        MarshallDeputy* x = new MarshallDeputy;
        m >> *x;
        auto shrd_ptr = shared_ptr<MarshallDeputy>(x);
        sync_data.push_back(shrd_ptr);
      }
      m >> sz;
      for(int i = 0; i < sz; i++){
        int32_t sz1;
        m >> sz1;
        vector<slotid_t> cur;
        for(int j = 0; j < sz1; j++){
          slotid_t x;
          m >> x;
          cur.push_back(x);
        }
        missing_slots.push_back(cur);
      } 
      return m;
    }
};

class SyncNoOpRequest : public Marshallable{
  public:
  int leader_id;
  ballot_t epoch;
  vector<slotid_t> sync_slots;
  SyncNoOpRequest(): Marshallable(MarshallDeputy::CMD_SYNCNOOP_PXS){

  }

  Marshal& ToMarshal(Marshal& m) const override {
    m << leader_id;
    m << epoch;
    m << (int32_t)sync_slots.size();
    for(int i = 0; i < sync_slots.size(); i++){
      m << sync_slots[i];
    }
    return m;
  }

  Marshal& FromMarshal(Marshal& m) override {
    m >> leader_id;
    m >> epoch;
    int32_t sz;
    m >> sz;
    for(int i = 0; i < sz; i++){
      slotid_t x;
      m >> x;
      sync_slots.push_back(x);
    }
    return m;
  }
};


class LogEntry : public Marshallable {
public:
  char* operation_ = nullptr;
  int length = 0;
  std::string log_entry;
  shared_ptr<char> operation_test;
  char len_v64[9];

  LogEntry() : Marshallable(MarshallDeputy::CONTAINER_CMD){
    bypass_to_socket_ = false;
  }

  virtual ~LogEntry() {
    if (operation_ != nullptr) delete operation_;
    operation_ = nullptr;
    //free(operation_test.get());
  }

  virtual Marshal& ToMarshal(Marshal&) const override;
  virtual Marshal& FromMarshal(Marshal&) override;
  size_t EntitySize() override {
    return sizeof(int) + length_as_v64() + length;
  }

  size_t length_as_v64(){
    v64 v_len = length;
    size_t bsize = rrr::SparseInt::dump(v_len.get(), len_v64);
    //Log_info("size of v64 obj is %d", bsize);
    return bsize;
  }

  size_t WriteToFd(int fd, size_t written_to_socket) override {
    size_t sz = 0, prev = written_to_socket;
    //Log_info("stepping here, writing length");
    if(written_to_socket < sizeof(int)){
      sz = track_write(fd, &length, sizeof(int), written_to_socket);
      if(sz > 0)written_to_socket += sz;
      assert(sz >= 0);
      if(written_to_socket < sizeof(int))return written_to_socket - prev;
    }
    //Log_info("stepping here, writing length_as_v64");
    size_t to_write = length_as_v64();
    if(written_to_socket < sizeof(int) + to_write){
      sz = track_write(fd, len_v64, to_write, written_to_socket - sizeof(int));
      if(sz > 0)written_to_socket += sz;
      assert(sz >= 0);
      if(written_to_socket < sizeof(int) + to_write)return written_to_socket - prev;
    }
    //Log_info("stepping here, writing data");
    if(written_to_socket < sizeof(int) + to_write + length) {
      if (true) {
        sz = track_write(fd, operation_test.get(), length, written_to_socket - sizeof(int) - to_write);
      } else {
        sz = track_write(fd, log_entry.c_str(), length, written_to_socket - sizeof(int) - to_write);
        //sz += blocking_write(fd, log_entry.c_str(), length);
      }
      if(sz > 0)written_to_socket += sz;
      assert(sz >= 0);
      if(written_to_socket < sizeof(int) + to_write + length)return written_to_socket - prev;
    }
    //Log_info("stepping here, written data entirely %lld, %lld", written_to_socket, EntitySize());
    assert(written_to_socket == EntitySize());
    assert(written_to_socket - prev >= 0);
    return written_to_socket - prev;
  }

  // void reset_write_offsets() override {
  //         written_to_socket = 0;
          
  // }
};

/*
inline rrr::Marshal& operator<<(rrr::Marshal &m, const LogEntry &cmd) {
  m << cmd.length;
  m << cmd.log_entry;
  return m;
}

inline rrr::Marshal& operator>>(rrr::Marshal &m, LogEntry &cmd) {
  m >> cmd.length;
  m >> cmd.log_entry;
  return m;
}
*/
class BulkPaxosCmd : public  Marshallable {
public:
  int32_t leader_id;
  vector<slotid_t> slots{};
  vector<ballot_t> ballots{};
  vector<shared_ptr<MarshallDeputy>> cmds{};
  char *serialized_slots = nullptr;

  BulkPaxosCmd() : Marshallable(MarshallDeputy::CMD_BLK_PXS) {
    bypass_to_socket_ = false;
  }
  virtual ~BulkPaxosCmd() {
      slots.clear();
      ballots.clear();
      cmds.clear();
  }
  Marshal& ToMarshal(Marshal& m) const override {
      m << (int32_t) leader_id;
      m << (int32_t) slots.size();
      for(auto i : slots){
          m << i;
      }
      m << (int32_t) ballots.size();
      for(auto i : ballots){
          m << i;
      }
      m << (int32_t) cmds.size();
      for (auto sp : cmds) {
        auto p = sp.get();
          m << *p;
      }
      return m;
  }

  Marshal& FromMarshal(Marshal& m) override {
      //return m;
      int32_t szs, szb, szc;
      m >> leader_id;
      m >> szs;
      for (int i = 0; i < szs; i++) {
          slotid_t x;
          m >> x;
          slots.push_back(x);
      }
      //return m;
      m >> szb;
      for (int i = 0; i < szs; i++) {
          ballot_t x;
          m >> x;
          ballots.push_back(x);
      }
      m >> szc;
      for (int i = 0; i < szc; i++) {
        auto x = new MarshallDeputy;
        m >> *x;
        auto sp_md = shared_ptr<MarshallDeputy>(x);
        cmds.push_back(sp_md);
      }
      return m;
  }

  size_t EntitySize() override {
    size_t sz = 0;
    sz += 4*sizeof(int32_t);
    for(int i = 0; i < slots.size(); i++){
      sz += sizeof(slotid_t);
      sz += sizeof(ballot_t);
      sz += cmds[i].get()->EntitySize();
    }
    return sz;
  }

  size_t serialize_slots_ballots(){
    int32_t batch = slots.size();
    size_t total_sz = 4*sizeof(int32_t) + batch*(sizeof(slotid_t) + sizeof(ballot_t));
    if(serialized_slots != nullptr){
      return total_sz;
    }
    serialized_slots = (char*)malloc(total_sz*sizeof(char));
    int wrt = 0;
    memcpy(serialized_slots + wrt, &leader_id, sizeof(int32_t));
    wrt += sizeof(int32_t);
    memcpy(serialized_slots + wrt, &batch, sizeof(int32_t));
    wrt += sizeof(int32_t);
    for(auto i : slots){
      memcpy(serialized_slots + wrt, &i, sizeof(slotid_t));
      wrt += sizeof(slotid_t);
    }
    memcpy(serialized_slots + wrt, &batch, sizeof(int32_t));
    wrt += sizeof(int32_t);
    for(auto i : ballots){
      memcpy(serialized_slots + wrt, &i, sizeof(ballot_t));
      wrt += sizeof(ballot_t);
    }
    memcpy(serialized_slots + wrt, &batch, sizeof(int32_t));
    wrt += sizeof(int32_t);
    return total_sz;
  }

  size_t WriteToFd(int fd, size_t written_to_socket) override {
    size_t to_write = serialize_slots_ballots(), sz = 0, prev = written_to_socket;
    //Log_info("written here %d %d", to_write, written_to_socket);
    if(written_to_socket < to_write){
      sz = track_write(fd, serialized_slots, to_write, written_to_socket);
      if(sz > 0){
        written_to_socket += sz;
      }
      verify(sz >= 0);
      if(written_to_socket < to_write)return written_to_socket - prev;
    }
    //Log_info("written here %d", written_to_socket);
    for (auto cmdsp : cmds) {
      to_write += cmdsp.get()->EntitySize();
      if(written_to_socket >= to_write)continue;
      sz = cmdsp.get()->WriteToFd(fd, written_to_socket - (to_write - cmdsp.get()->EntitySize()));
      //std::cout << "should have written bytes "<< sz << std::endl;
      if(sz > 0){
        written_to_socket += sz;
      }
      verify(sz >= 0);
      //Log_info("written here %d %d", written_to_socket, EntitySize());
      verify(written_to_socket - prev >= 0);
      if(written_to_socket < to_write)return written_to_socket - prev;
    }
    //free(serialized_slots);
    //Log_info("written to socket %d  and size is %d", written_to_socket, EntitySize());
    verify(written_to_socket == EntitySize());
    return written_to_socket - prev;
  }

  // void reset_write_offsets() override {
	 //  written_to_socket = 0;
	 //  for(auto cmdsp : cmds){
	 //     cmdsp.get()->reset_write_offsets();
	 //  }
  // }
};

class PaxosWorker {
private:
  inline void _Submit(shared_ptr<Marshallable>);
  inline void _BulkSubmit(shared_ptr<Marshallable>, int);

  rrr::Mutex finish_mutex{};
  rrr::CondVar finish_cond{};
  std::function<void(const char*, int)> callback_ = nullptr;
  std::function<void(const char*&, int, int)> callback_par_id_ = nullptr;
  std::function<unsigned long long int(const char*&, int, int, std::queue<std::tuple<unsigned long long int, int, int, const char *>> &)> callback_par_id_return_ = nullptr;
  vector<Coordinator*> created_coordinators_{};
  vector<shared_ptr<Coordinator>> created_coordinators_shrd{};
  struct timeval t1;
  struct timeval t2;

public:
  std::atomic<int> n_current{0};
  std::atomic<int> n_submit{0};
  std::atomic<int> n_tot{0};
  SubmitPool* submit_pool = nullptr;
  rrr::PollMgr* svr_poll_mgr_ = nullptr;
  vector<rrr::Service*> services_ = {};
  rrr::Server* rpc_server_ = nullptr;
  base::ThreadPool* thread_pool_g = nullptr;
  // for microbench
  std::atomic<int> submit_num{0};
  int tot_num = 0;
  int submit_tot_sec_ = 0;
  int submit_tot_usec_ = 0;
  int cur_epoch;
  int is_leader;
  int bulk_writer = 0;
  int bulk_reader = 0;
  

  rrr::PollMgr* svr_hb_poll_mgr_g = nullptr;
  ServerControlServiceImpl* scsi_ = nullptr;
  rrr::Server* hb_rpc_server_ = nullptr;
  base::ThreadPool* hb_thread_pool_g = nullptr;

  Config::SiteInfo* site_info_ = nullptr;
  std::queue<std::tuple<unsigned long long int, int, int, const char *>> un_replay_logs_ ;  // latest_commit_id, status, len, log
  Frame* rep_frame_ = nullptr;
  TxLogServer* rep_sched_ = nullptr;
  Communicator* rep_commo_ = nullptr;
  std::recursive_mutex mtx_worker_submit{};
  static moodycamel::ConcurrentQueue<shared_ptr<Coordinator>> coo_queue;
  static std::queue<shared_ptr<Coordinator>> coo_queue_nc;
  moodycamel::ConcurrentQueue<Marshallable*> replay_queue;
  vector<shared_ptr<Coordinator>> all_coords = vector<shared_ptr<Coordinator>>(1000000, nullptr);
  rrr::Mutex nc_submit_l_;
  std::recursive_mutex election_state_lock;
  const unsigned int cnt = bulkBatchCount;
  pthread_t bulkops_th_;
  pthread_t replay_th_;
  bool stop_flag = false;
  bool stop_replay_flag = true;

  void SetupHeartbeat();
  void InitQueueRead();
  void SetupBase();
  int  deq_from_coo(vector<shared_ptr<Coordinator>>&);
  void SetupService();
  void SetupCommo();
  void ShutDown();
  void Next(Marshallable&);
  void WaitForSubmit();
  void IncSubmit();
  void BulkSubmit(const vector<shared_ptr<Coordinator>>&);
  void AddAccept(shared_ptr<Coordinator>);
  void AddAcceptNc(shared_ptr<Coordinator>);
  void AddReplayEntry(Marshallable&);
  void submitJob(std::shared_ptr<Job>);
  int SendBulkPrepare(shared_ptr<BulkPrepareLog>);
  int SendHeartBeat(shared_ptr<HeartBeatLog>);
  int SendSyncLog(shared_ptr<SyncLogRequest>);
  int SendSyncNoOpLog(shared_ptr<SyncNoOpRequest>);
  static void* StartReadAccept(void*);
  static void* StartReplayRead(void*);
  static void* StartReadAcceptNc(void*);
  PaxosWorker();
  ~PaxosWorker();

  static const uint32_t CtrlPortDelta = 10000;
  void WaitForShutdown();
  bool IsLeader(uint32_t);
  bool IsPartition(uint32_t);

  void Submit(const char*, int, uint32_t);
  void register_apply_callback(std::function<void(const char*, int)>);
  void register_apply_callback_par_id(std::function<void(const char*&, int, int)>);
  void register_apply_callback_par_id_return(std::function<unsigned long long int(const char*&, int, int, std::queue<std::tuple<unsigned long long int, int, int, const char *>> &)>);
  rrr::PollMgr * GetPollMgr(){
      return svr_poll_mgr_;
  }
};

extern vector<shared_ptr<PaxosWorker>> pxs_workers_g;

class ElectionState {
  ElectionState(){}
public: 
  std::recursive_mutex election_mutex{};
  pthread_t election_th_;
  pthread_t heartbeat_th_;
  bool running = true;
  int timeout = 1; // in seconds
  int heartbeat_timeout = 300; // in milliseconds
  int send_prep_anyway_timeout = 1;
  int cur_epoch = 0;
  int cur_state = 0; // 0 Follower, 1 Leader
  int machine_id = -1;
  int leader_id = -1;
  rrr::Mutex election_state;
  rrr::CondVar election_cond{};
  rrr::Mutex stuff_after_election_mutex_;
  rrr::CondVar stuff_after_election_cond_{};
  timepoint lastseen = std::chrono::high_resolution_clock::now();
  timepoint last_prep_sent = std::chrono::high_resolution_clock::now();

  //void operator=(const ElectionState &) = delete;

  static shared_ptr<ElectionState> instance(){
    static shared_ptr<ElectionState> instance_ptr(new ElectionState);
    return instance_ptr;
  }

  int get_machine_id(){
    return machine_id;
  }

  // not to be called while state lock acquired.
  int get_consistent_epoch(){
    int x;
    state_lock();
    x = cur_epoch;
    state_unlock();
    return x;
  }

  bool is_leader(){
    int x;
    state_lock();
    x = cur_state;
    state_unlock();
    return x;
  }

  int get_epoch(){
    return cur_epoch;
  }

  void state_lock(){
    election_mutex.lock();
  }

  void state_unlock(bool sleep = false){
    election_mutex.unlock();
    if(sleep)
        sleep_timeout();
  }

  int set_epoch(int val = -1){
    if(val == -1){
      return ++cur_epoch;
    } else{
      cur_epoch = val;
    }
    assert(val >= cur_epoch);
    return cur_epoch;
  }

  void reset_timeout(){
    timeout = rand()%4;
  }

  void sleep_timeout(){
    std::this_thread::sleep_for(std::chrono::seconds(timeout));
  }

  void sleep_heartbeat(){
    std::this_thread::sleep_for(std::chrono::milliseconds(heartbeat_timeout));
  }

  void set_state(int val){
    cur_state = val;
  }

  void set_leader(int val){
    //if(val != 0)
//	Log_info("Leader being set %d", val);
    leader_id = val;
  }

  void set_lastseen(){
    lastseen = std::chrono::high_resolution_clock::now();
  }

  void set_bulkprep_time(){
    last_prep_sent = std::chrono::high_resolution_clock::now();
  }

  bool did_not_see_leader(){
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-lastseen;
    return (double)timeout < diff.count();
  }

  bool did_not_send_prep(){
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end-last_prep_sent;
    return (double)send_prep_anyway_timeout < diff.count();
  }

  void step_down(int epoch){
    state_lock();
    if(cur_epoch > epoch){
      state_unlock();
      return;
    }
    set_state(0);
    leader_id = -1;
    set_epoch(epoch);
    for(int i = 0 ; i < pxs_workers_g.size(); i++){
      pxs_workers_g[i]->election_state_lock.lock();
      pxs_workers_g[i]->cur_epoch = epoch;
      pxs_workers_g[i]->is_leader = 1;
      pxs_workers_g[i]->election_state_lock.unlock();
    }
    state_unlock();
  }
};

} // namespace janus
