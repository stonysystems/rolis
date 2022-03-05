//
// Created by mrityunjay kumar on 2019-10-25.
//

#if !defined(LOG_TO_FILE)
#include "ReplayDB.h"
#endif
#include "gperftools/profiler.h"
#include <random>
#include <algorithm>
#include <unordered_set>
#include "ThreadPool.h"

#define  MAXLINE 100

#define MAC 0
#define MULTI_TH_CONSUMERS 0


static bool init_done= false;

long double print_time(struct timeval tv1, struct timeval tv2) {
  	auto ans = (tv2.tv_sec-tv1.tv_sec) + (tv2.tv_usec-tv1.tv_usec)/1000000.0;
    printf("%f\n", ans);
  	return ans;
}

#ifndef MASS_DIRECT
void example_function(ds &h, const container_hello* a)
{
    std::atomic<int> fail_counts{0};
    std::atomic<int> success_counts{0};
    std::atomic<int> inserts{0};
    std::atomic<int> deletes{0};
    std::atomic<int> failed{0};
//	std::cout << "example_function threadID: "<<a->GetThreadID ()<<std::endl;
	{
		TThread::set_id(a->GetThreadID ());
		Sto::update_threadid();
		actual_ds::thread_init();
	}
//	std::cout << "example_function start1"<<std::endl;

//    std::cout << a->GetFilename () << " : " << a->GetFileStartPos() << " : " << a->GetFileEndPos() << " Pop [start]" <<std::endl;
    getFileContent(a, h ,success_counts, inserts, deletes,failed);
//    if(read_ok)
//    	std::cout << "Table Name : " << a->GetPreetyFName () << " OP Stats : {" <<inserts << " ,"<<   deletes << " ,"<< failed  << "}"<< std::endl;
//    global_success_counts+=success_counts;
//    cout << "==============="<<endl;
}

//void set_cpu(std::thread& thd_,int cpu_id){
//  cpu_set_t cpuset;
//  CPU_ZERO(&cpuset);
//  CPU_SET(cpu_id, &cpuset);
//  pthread_setaffinity_np(thd_.native_handle(),sizeof(cpu_set_t), &cpuset);
//}

#if 0
bool replay_file(const container_hello* arg, ds &h,
	std::atomic<int> &fail_counts, std::atomic<int> &success_counts)
{
	std::string fileName = arg->GetFilename ();
    unsigned long long int cid = 0;
	size_t rw=0;
    size_t ul_len = sizeof (unsigned long long int);
	unsigned long long int k =0;
    unsigned long long int v = 0;
    unsigned long long int table_id=0;
    unsigned long long int delete_true=0;
    unsigned long long int insert_true=0;
	size_t covered =0;
    std::string value="0";
    size_t org_count=0;
    size_t retried_count=0;
    size_t ret;
    size_t end_pos = arg->GetFileEndPos();
  	char *buffer;
  	void *ptr;
  #ifdef TEST_REPLAY_SPEED_FEAT
  	auto logger = new OutputDataSerializer(GetNextThreadID(),
  		"/home/jay/replay_logs/"+std::to_string(arg->run_thread_count())+"/",
  		true);
  #endif
  	
  	size_t start_pos = arg->GetFileStartPos ();
    size_t sz = end_pos-start_pos;
    int fd = open(arg->GetFilename ().c_str(), O_RDONLY);
    sz = ((sz/512)+1)*512;
    if(fd<=0){
	  return false;
    }
    lseek (fd, start_pos,SEEK_SET);
    
    if ((ret = posix_memalign(&ptr, 512, sz)) != 0) {
      std::cout << "ERROR ret=0"<< std::endl;
	  return false;
	}
    
    buffer = (char *)ptr;
    ret = read(fd, buffer, sz);
//    sz = ret;
	// TODO:  we missed some of last info
//	std::cout << "How Much Needed : " << sz << " Actual : " << ret << std::endl;
    if(ret<=0){
      	std::cout << "ERROR ret=1"<< std::endl;
        return false;
    }
//    std::cout << "End pos : "<< end_pos << std::endl;
    // procesxs
    rw =0;
    
//    std::lock_guard<std::mutex> guard(g_pages_mutex);
  {
	
	for (size_t i = 0; i < sz / (ul_len * 6) && rw < end_pos; i++) {
	  #ifdef TEST_REPLAY_SPEED_FEAT
	  logger->BloopLog ();
	  #endif
	  cid = 0;
	  k = 0;
	  v = 0;
	  delete_true=0;
	  insert_true=0;
	  try {
		memcpy ((char *) &cid, buffer + rw, ul_len);
		rw += ul_len;
		
		memcpy ((char *) &k, buffer + rw, ul_len);
		rw += ul_len;
		
		memcpy ((char *) &v, buffer + rw, ul_len);
		rw += ul_len;
		
	    memcpy ((char *) &table_id, buffer + rw, ul_len);
		rw += ul_len;
		
		memcpy ((char *) &delete_true, buffer + rw, ul_len);
		rw += ul_len;
		
		memcpy ((char *) &insert_true, buffer + rw, ul_len);
		rw += ul_len;
		
//		std::cout << "i=" << i+1 << " cid=" << cid<< " k= "<<k << " v="<<v<<std::endl;
//		std::cout << "i= " << i+1 << " TABLE NAME= " << table_id  << " cid=" << cid<< " k= "<<k << " v="<< v << " Insert= " << insert_true << " Delete= " << delete_true <<std::endl;

		std::string encoded = Encode (std::to_string (cid), std::to_string (v));
		try {
		  TRANSACTION
				{
				  if(insert_true) {
					if (h[table_id].transGet (std::to_string (k), value)) {
					  unsigned long old_cid = std::stoul (GetCid (value));
					  if (old_cid < cid) {
//                              			h.transUpdate (std::to_string(k), encoded);
						h[table_id].transPut (std::to_string (k), encoded);
						retried_count++;
					  }
					} else {
					  h[table_id].transPut (std::to_string (k), encoded);
					  org_count++;
					}
				  }else if(delete_true){
					if (!h[table_id].transDelete (std::to_string (k))) {
					  fail_counts++;
					}
				  }
				}
		  RETRY(false);
		} catch (...) {
		  fail_counts++;
		}
		success_counts+=1;
	  } catch (...) {
		// dont count this
		covered+=(ul_len*6);
      	if(covered>end_pos)
		  break;
	  }
	}
	//Close The File
	free (ptr);
	close (fd);
	#ifdef TEST_REPLAY_SPEED_FEAT
	logger->flush (true);
	#endif
	return true;
  }
}
#endif
#endif

class mymy_scan_callback {
  public:
    virtual ~mymy_scan_callback() {}
    // XXX(stephentu): key is passed as (const char *, size_t) pair
    // because it really should be the string_type of the underlying
    // tree, but since abstract_ordered_index is not templated we can't
    // really do better than this for now
    //
    // we keep value as std::string b/c we have more control over how those
    // strings are generated
    virtual bool invoke(const char *keyp, size_t keylen,
                        const std::string &value) = 0;
  };

class my_key_callback : public mymy_scan_callback {
public:
  my_key_callback(){count=0;}

  virtual bool invoke(
      const char *keyp, size_t keylen,
      const std::string &value)
  {
    count++;
    std::cout << std::string(keyp,keylen) << value <<std::endl;
    return true;
  }
  inline const size_t
  getCount() const
  {
    return count;
  }
 private:
  size_t count;
};


#define ALWAYS_INLINE2 __attribute__((always_inline))
#define INVARIANT2(expr) ((void)0)

#if 0
class str_arena2 {
public:

  static const size_t PreAllocBufSize = 1024;
  static const size_t NStrs = 1024*1024;

  static const size_t MinStrReserveLength = 2 * 64;
  static_assert(PreAllocBufSize >= MinStrReserveLength, "xx");

  str_arena2()
    : n(0)
  {
    for (size_t i = 0; i < NStrs; i++)
      strs[i].reserve(PreAllocBufSize);
  }

  // non-copyable/non-movable for the time being
  str_arena2(str_arena2 &&) = delete;
  str_arena2(const str_arena2 &) = delete;
  str_arena2 &operator=(const str_arena2 &) = delete;

  inline void
  reset()
  {
    n = 0;
    overflow.clear();
  }

  // next() is guaranteed to return an empty string
  std::string *
  next()
  {
    if (likely(n < NStrs)) {
      std::string * const px = &strs[n++];
      px->clear();
      INVARIANT2(manages(px));
      return px;
    }
    // only loaders need this- and this allows us to use a unified
    // str_arena2 for loaders/workers
    overflow.emplace_back(new std::string);
    ++n;
    return overflow.back().get();
  }

  inline std::string *
  operator()()
  {
    return next();
  }

  void
  return_last(std::string *px)
  {
    INVARIANT2(n > 0);
    --n;
  }

  bool
  manages(const std::string *px) const
  {
    return manages_local(px) || manages_overflow(px);
  }

private:

  bool
  manages_local(const std::string *px) const
  {
    if (px < &strs[0])
      return false;
    if (px >= &strs[NStrs])
      return false;
    return 0 == ((reinterpret_cast<const char *>(px) -
                  reinterpret_cast<const char *>(&strs[0])) % sizeof(std::string));
  }

  bool
  manages_overflow(const std::string *px) const
  {
    for (auto &p : overflow)
      if (p.get() == px)
        return true;
    return false;
  }

private:
  std::string strs[NStrs];
  std::vector<std::unique_ptr<std::string>> overflow;
  size_t n;
};
class scoped_str_arena2 {
public:
  scoped_str_arena2(str_arena2 *arena)
    : arena(arena)
  {
  }

  scoped_str_arena2(str_arena2 &arena)
    : arena(&arena)
  {
  }

  scoped_str_arena2(scoped_str_arena2 &&) = default;

  // non-copyable
  scoped_str_arena2(const scoped_str_arena2 &) = delete;
  scoped_str_arena2 &operator=(const scoped_str_arena2 &) = delete;


  ~scoped_str_arena2()
  {
    if (arena) {
	  arena->reset ();
//	  std::cout << "dtor scoped_str_arena2" << endl;
	}
  }

  inline ALWAYS_INLINE2 str_arena2 *
  get()
  {
    return arena;
  }

private:
  str_arena2 *arena;
};
#endif

#ifndef MASS_DIRECT
void validate_flow(std::map<std::string, long> &mapper,new_ds& h, std::vector<one_container*>& files_names){
	// std::cout << "------------\nVERIFY START 0\n------------" << std::endl;
	std::map<long,std::string> r_mapper={};
	std::map<std::pair<long,unsigned long long int>,const one_container*> key_value_pairs={};
	std::map<long, std::pair<unsigned long long int,unsigned long long int>> stats_by_table={};
//	size_t t_cases=0;
	size_t passed=0;
	size_t extra=0;

	for(auto &t_iter:mapper){
	  r_mapper[t_iter.second] = t_iter.first;
	  stats_by_table[t_iter.second]= std::make_pair(0,0);
	}

	//for(auto &a: files_names){
	  // retrieve list
	  //auto all_mapped_lists = a->GetItems ();
	  for(auto each_container: files_names){
		  auto mapped_pair = std::make_pair(each_container->k,each_container->k);
		  if(key_value_pairs.find(mapped_pair)==key_value_pairs.end()) {
			key_value_pairs[mapped_pair] = each_container;
		  }
		  else{
		    if(key_value_pairs[mapped_pair]->cid < each_container->cid
		    && key_value_pairs[mapped_pair]->k == each_container->k) {
		      key_value_pairs[mapped_pair] = each_container;
		    }
		}
	  }
	//}
	// std::cout << "------------\nVERIFY START 1\n------------" << std::endl;
	for(auto each: key_value_pairs){
	  	  NO_PAXOS_TRANSACTION
			{
	  	      std::string value;
	  	      auto updated = stats_by_table[each.second->table_id];

	  	      if(each.second->insert_true) {
	  	        updated.first++;
				if (h[each.second->table_id].transGet (std::to_string (each.second->k), value)) {
				  passed++;
				  updated.second++;
				}else{
				  // std::cout << "[Insert-fail] "<< each.second->k <<":"<< each.second->v << ":"<<  each.second->cid << ":" <<  each.second->insert_true <<  each.second->delete_true << " [val] " << value << std::endl;
	  	          extra++;
				}
			  }else if(each.second->delete_true){
	  	        updated.first++;
	  	        if (h[each.second->table_id].transGet (std::to_string (each.second->k), value)) {
	  	          auto retrieved_value = GetDecoded(value);
				  unsigned long old_value = std::stoul (retrieved_value[1]);
				  if(old_value!=999) {
					// std::cout << "[Delete-fail] " << each.second->k << ":" << each.second->v << ":" << each.second->cid
//							  << ":" << each.second->insert_true << each.second->delete_true << " [val] " << value
//							  << std::endl;
					extra++;
				  }else{
				    passed++;
				  	updated.second++;
				  }
				}else{
	  	          passed++;
				  updated.second++;
	  	        }
	  	      }
			  stats_by_table[each.second->table_id] = updated;
			}
			NO_PAXOS_RETRY(true);
	}

//	std::cout << "------------\nVERIFY START 2\n------------" << std::endl;

	for(auto &tby_table:stats_by_table){
		if(tby_table.second.second!=tby_table.second.first){
//		  std::cout << "Correctness check failed" << std::endl;
//		  std::cout << "Table ID: " << tby_table.first << " Table Name : " << r_mapper[tby_table.first] << " Status:["<< tby_table.second.second<< " / " << tby_table.second.first << "]" << std::endl<< std::endl<< std::endl;
		  exit(1);
		}
	}
	std::cout << std::endl << "\n\nCorrectness check passed [OK]" << std::endl;

//	cout << "------------\n------------" <<endl;
}

void validate_flow(std::map<std::string, long> &mapper,ds& h, std::vector<container_hello*>& files_names){
	// std::cout << "------------\nVERIFY START 0\n------------" << std::endl;
	std::map<long,std::string> r_mapper={};
	std::map<std::pair<long,unsigned long long int>,const one_container*> key_value_pairs={};
	std::map<long, std::pair<unsigned long long int,unsigned long long int>> stats_by_table={};
//	size_t t_cases=0;
	size_t passed=0;
	size_t extra=0;
	
	for(auto &t_iter:mapper){
	  r_mapper[t_iter.second] = t_iter.first;
	  stats_by_table[t_iter.second]= std::make_pair(0,0);
	}
	
	for(auto &a: files_names){
	  // retrieve list
	  auto all_mapped_lists = a->GetItems ();
	  for(auto each_container: all_mapped_lists){
		  auto mapped_pair = std::make_pair(each_container->k,each_container->k);
		  if(key_value_pairs.find(mapped_pair)==key_value_pairs.end()) {
			key_value_pairs[mapped_pair] = each_container;
		  }
		  else{
		    if(key_value_pairs[mapped_pair]->cid < each_container->cid
		    && key_value_pairs[mapped_pair]->k == each_container->k) {
		      key_value_pairs[mapped_pair] = each_container;
		    }
		}
	  }
	}
	// std::cout << "------------\nVERIFY START 1\n------------" << std::endl;
	for(auto each: key_value_pairs){
	  	  NO_PAXOS_TRANSACTION
			{
	  	      std::string value;
	  	      auto updated = stats_by_table[each.second->table_id];
	  	      
	  	      if(each.second->insert_true) {
	  	        updated.first++;
				if (h[each.second->table_id]->transGet (std::to_string (each.second->k), value)) {
				  passed++;
				  updated.second++;
				}else{
				  // std::cout << "[Insert-fail] "<< each.second->k <<":"<< each.second->v << ":"<<  each.second->cid << ":" <<  each.second->insert_true <<  each.second->delete_true << " [val] " << value << std::endl;
	  	          extra++;
				}
			  }else if(each.second->delete_true){
	  	        updated.first++;
	  	        if (h[each.second->table_id]->transGet (std::to_string (each.second->k), value)) {
	  	          auto retrieved_value = GetDecoded(value);
				  unsigned long old_value = std::stoul (retrieved_value[1]);
				  if(old_value!=999) {
					// std::cout << "[Delete-fail] " << each.second->k << ":" << each.second->v << ":" << each.second->cid
//							  << ":" << each.second->insert_true << each.second->delete_true << " [val] " << value
//							  << std::endl;
					extra++;
				  }else{
				    passed++;
				  	updated.second++;
				  }
				}else{
	  	          passed++;
				  updated.second++;
	  	        }
	  	      }
			  stats_by_table[each.second->table_id] = updated;
			}
			NO_PAXOS_RETRY(true);
	}
	
//	std::cout << "------------\nVERIFY START 2\n------------" << std::endl;
	
	for(auto &tby_table:stats_by_table){
		if(tby_table.second.second!=tby_table.second.first){
//		  std::cout << "Correctness check failed" << std::endl;
//		  std::cout << "Table ID: " << tby_table.first << " Table Name : " << r_mapper[tby_table.first] << " Status:["<< tby_table.second.second<< " / " << tby_table.second.first << "]" << std::endl<< std::endl<< std::endl;
		  exit(1);
		}
	}
//	std::cout << std::endl << "\n\nCorrectness check passed [OK]" << std::endl;
	
//	cout << "------------\n------------" <<endl;
}


void do_verify(std::map<std::string, long>& table_map,ds &database,std::vector<container_hello*>& verify_objs){
//  std::cout << "Validation start"<<std::endl;
  validate_flow(table_map, database, verify_objs);
//  std::cout << "Validation stop"<<std::endl;
}

void replay_process(char *log, size_t len, ds &database,int nthreads,std::map<std::string, long>& table_map){
	
	static bool once = [](){
		{
		  Transaction::epoch_advance_callback = [] (unsigned) {
            // just advance blindly because of the way Masstree uses epochs
            globalepoch++;
	};
	  // someone has to do this (they don't provide us with a general init callback)
	  std::cout << "static_init done once" << std::endl;
//	  actual_ds::static_init();
	  // need this too
//	  pthread_t advancer;
//	  pthread_create(&advancer, NULL, Transaction::epoch_advancer, NULL);
//	  pthread_detach(advancer);
		}
	  return true;
	} ();
  	
  	
  	if(!init_done){
  	    for (auto &each: table_map) {
		  auto table_id = each.second;
		  auto *hh = new actual_ds();
		  hh->set_table_name (table_id);
		  database[table_id] = hh;
  		}
  	    init_done = true;
  	}
  
  	if(len <=0){
  	  std::cout << "Empty Strings : " << std::endl;
	  return;
  	}
  
  	int wthreads=1;
  	std::vector<std::thread> thread_pool;
    thread_pool.reserve(nthreads);
  	// TODO table_map

  	std::vector<long> table_names;

	//table_names.reserve(table_map.size());
	for(auto &each:table_map){
//	  std::cout << "Table ID:-> Key: " << each.first << " :: Value :" << each.second << "\n";
	  table_names.push_back(each.second);
	}
//	exit(0);
	// TODO database init
	
  #if 1
	
	//TODO files_names
	std::vector<container_hello*> files_names = {};
//	gettimeofday(&tv1, NULL);
	load_objects_from_buffer(log, len,wthreads,nthreads,table_map,files_names);
//	gettimeofday(&tv2, NULL);
//	std::cout << "[process-MGR] added " << files_names.size() << std::endl;
	
	Function_pool func_pool;
    for (int i = 0; i < nthreads; i++)
    {
		thread_pool.emplace_back(&Function_pool::infinite_loop_func, &func_pool);
    }
    
    for (auto &a:files_names) {
	  func_pool.push (example_function, std::ref (database), a);
	}
 
	func_pool.done();
    for (auto & i : thread_pool)
	{
		i.join();
	}
    std::cout << "Finished processing across" << std::endl;
    //do_verify(table_map, database, files_names);
    unsigned long long int txns_total=0;
    for(auto &a:files_names){
      txns_total+=a->GetContainerCounts ();
      a->free_all ();
      delete a;
    }
//    all_txns_recorded.emplace_back (files_names);
    std::cout << "Txns : " << txns_total << std::endl;
  #endif
    std::cout << "Finished" << std::endl;
}

void start_replay_processing(int file_count,int replay_thr_count,ds& database){
  std::string file_name = std::string (LOG_FOLDER_DIR) + \
								std::string ("Log-ThreadID-") + \
								std::to_string (file_count) +std::string(".txt" );
  std::cout << "Start processing file : " << file_name << std::endl;
  size_t sz = get_file_size(file_name);
  size_t original_sz = sz;
  size_t covered = 0;
  void *ptr=NULL;
  char *buffer;
  size_t rw=0;
  size_t ret=0;
  size_t total_entries_local = 0;
  std::set<unsigned long long int> unique_keys;
  int fd = open(file_name.c_str(), O_RDONLY);
  
  ptr = (void *) malloc (sz);
  buffer = (char *) ptr;
  ret = read (fd, buffer, sz);
  
  std::vector<long> table_names={10001,10002,10003,10004,10005,10006,10007,10008,10009,10010,10011,10012};
  	std::string table_info_file_name = std::string(INFO_LOG_FOLDER_NAME) + "/Log-ThreadID-9999.txt";
	std::map<std::string, long> table_map = {};
	bool pre_process_okay = fileToMap(table_info_file_name,table_map);
	if(!pre_process_okay)
	  exit(1);
	
  // init a database
//  for(auto &table_id: table_names){
//      actual_ds hh;
//      hh.set_table_name (table_id);
//      database[table_id] = hh;
//  }

  replay_process(buffer, ret,std::ref(database),replay_thr_count,table_map);
}


void replay_in_same_thread(char *log, size_t len, new_ds& database,std::map<std::string, long>& table_map, int tid){
  if(len <=0){
	std::cout << "Empty Strings : " << std::endl;
	return;
  }
   #if 1
	
	//TODO files_names
//	std::vector<container_hello*> files_names = {};
	std::vector<one_container*> files_names_t = {};

  //void load_objects_from_buffer_no_table(char *buffer, int length,int wthreads,int nthreads, const std::map<std::string,
  //	long>& table_map,std::vector<one_container*>& all_containers)
//	gettimeofday(&tv1, NULL);
	auto startTime = std::chrono::steady_clock::now();
//	load_objects_from_buffer(log, len,1,1,table_map,files_names_t);
	load_objects_from_buffer_no_table(log, len,1,1,table_map,files_names_t);
	auto endTime = std::chrono::steady_clock::now();
    TimerMapper::add_time("load_objects_from_buffer_"+std::to_string(tid),std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count(),1000.0*1000.0);
	
	std::cout << "[DEBUG] adding, thread_id: " << std::to_string(tid) << ", " << files_names_t.size() << " jobs" << std::endl;
	
    startTime = std::chrono::steady_clock::now();
    //std::cout << "tid: id, " << std::to_string(tid) << std::endl ;
   static bool once = [tid](){  // FLAGXXX
	    TThread::set_id (tid);
            Sto::update_threadid ();
            actual_ds::thread_init ();
         return true;
        } ();
       
       //OutputDataSerializer::GetLogger (TThread::id ())->Log ((char *)log, len);
//	for(auto &container:files_names) {
//	  TThread::set_id (container->GetThreadID ());
//	  Sto::update_threadid ();
//	  actual_ds::thread_init ();
//	  std::atomic<int> fail_counts{0};
//	  std::atomic<int> success_counts{0};
//	  std::atomic<int> inserts{0};
//	  std::atomic<int> deletes{0};
//	  std::atomic<int> failed{0};
//	  getFileContentNew(container,database,success_counts,inserts,deletes,failed);
//	}
	std::atomic<int> fail_counts{0};
	std::atomic<int> success_counts{0};
	std::atomic<int> inserts{0};
	std::atomic<int> deletes{0};
	std::atomic<int> failed{0};
	getFileContentNew_t(files_names_t,database,success_counts,inserts,deletes,failed);

    //validate_flow(table_map, database, files_names_t);	
  
    endTime = std::chrono::steady_clock::now();
    TimerMapper::add_time("add_to_masstree_"+std::to_string(tid),std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count(),1000.0*1000.0);
 	
  #endif
 	
}
#endif

//std::mutex c_mtx;           // locks access to counter
//inline size_t txns_count = 0;
std::atomic<size_t> txns_count {0};
std::atomic<size_t> put_count {0};

void replay_in_same_thread_direct(char *log, size_t len, new_directs& database,std::map<std::string, long>& table_map, int tid)
{
  if(len <=0){
	std::cout << "Empty Strings : " << std::endl;
	return;
  }
   #if 1
	
	//TODO files_names
	std::vector<one_container*> files_names_t = {};

  //void load_objects_from_buffer_no_table(char *buffer, int length,int wthreads,int nthreads, const std::map<std::string,
  //	long>& table_map,std::vector<one_container*>& all_containers)
//	gettimeofday(&tv1, NULL);
//	auto startTime = std::chrono::steady_clock::now();
//	load_objects_from_buffer(log, len,1,1,table_map,files_names_t);
	auto _tcount = load_objects_from_buffer_no_table(log, len,1,1,table_map,files_names_t);
//	auto endTime = std::chrono::steady_clock::now();
//    TimerMapper::add_time("load_objects_from_buffer_"+std::to_string(tid),std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count(),1000.0*1000.0);
	
//	std::cout << "[DEBUG] adding " << files_names.size() << " jobs" << std::endl;
	
//    startTime = std::chrono::steady_clock::now();
//	std::atomic<int> fail_counts{0};
//	std::atomic<int> success_counts{0};
//	std::atomic<int> inserts{0};
//	std::atomic<int> deletes{0};
//	std::atomic<int> failed{0};
//	std::atomic<size_t> txns_count_this(0);
//	std::cout << "Push " << _tcount << " txns now" << std::endl;
	
	auto _put = getFileContentNew_optimized (files_names_t,database);
    //validate_flow(table_map, database, files_names_t);
  	for(auto &a:files_names_t){
      delete a;
    }
//    endTime = std::chrono::steady_clock::now();
    txns_count += _tcount;
    put_count += _put;
//    auto timeDiff = std::chrono::duration_cast<std::chrono::microseconds>(endTime - startTime).count();
//    TimerMapper::add_time("replay_time_"+std::to_string(tid),timeDiff,1000.0*1000.0);
//    TimerMapper::add_time("total_replay_time_"+std::to_string(tid),timeDiff,1000.0*1000.0);
//	TimerMapper::add_time("txns_at_thread_id_"+std::to_string(tid),_tcount,1);
//	TimerMapper::add_time("throughput_txns_at_thread_id_"+std::to_string(tid),float(_tcount)/float(timeDiff)/1000.0*1000.0,1);
//    std::cout << "Stats :: " << txns_count << " processed" << std::endl;
	
  #endif
}

uint64_t timeSinceEpochMillisecX() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int replay_in_same_thread_wrapper(int thread_id, int offset,
				  const std::vector<std::pair<char *,int>>& buffers,
                                  bool end_true, int stride,
                                  new_directs database,
                                  std::map<std::string, long> table_map)
{
  std::cout << "Thread gets started: " << thread_id << ", coreid: " << sched_getcpu() << std::endl;
  auto start = timeSinceEpochMillisecX(); 
  std::vector <std::pair<char *,int>>::const_iterator it = buffers.begin () + offset;
  size_t total_bytes = 0;
  if(end_true) {
  	for(;it != buffers.end(); it++ ) {
		replay_in_same_thread_direct((*it).first, (*it).second, std::ref (database),table_map, thread_id);
		total_bytes += (*it).second;
  	}
  } else {
  	for(;it != buffers.begin () + offset + stride ; ++it) {
		replay_in_same_thread_direct((*it).first, (*it).second, std::ref (database),table_map, thread_id);
		total_bytes += (*it).second;
  	}
  }
  auto end = timeSinceEpochMillisecX(); 
  std::cout << "[cost of second: " << (end - start)/1000.0  << ", total bytes: " << total_bytes / 27 <<"]Thread: " << thread_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
  return 0 ;
}

size_t Treplay_in_same_thread_wrapper_pointer_even(int thread_id,
                                                  size_t offset,
                                                  char *base,
                                                  size_t entries_cnt_per_core,
                                                  new_directs& database) {
//    std::cout << "Thread " << thread_id << " gets started, offset: " << offset << std::endl;
//    auto start = timeSinceEpochMillisecX();
    size_t thread_put = 0;
    size_t chunk = 512;
    size_t i = 0;
    while (i < entries_cnt_per_core) {
        char *init = base + offset + i * 27 ;

        if (entries_cnt_per_core - i > chunk) {
            i += chunk ;
        } else {
            chunk = entries_cnt_per_core - i ;
            i = entries_cnt_per_core ;
        };

        thread_put += getFileContentNew_optimized_pointer(init, chunk, database) ;
    }
//    auto end = timeSinceEpochMillisecX();
//    std::cout << "[cost of second: " << (end - start)/1000.0  << ", # of PUTs: " << thread_put <<"]Thread: " << thread_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
    return thread_put ;
}

size_t replay_in_same_thread_wrapper_pointer_even(int thread_id,
                                                  size_t offset,
                                                  char *base,
                                                  size_t entries_cnt_per_core,
                                                  new_directs database) {
    //std::cout << "Thread " << thread_id << " gets started, offset: " << offset << std::endl;
    //auto start = timeSinceEpochMillisecX();
    size_t thread_put = 0;
    size_t chunk = 512;

    size_t i = 0;
    while (i < entries_cnt_per_core) {
        char *init = base + offset + i * 27 ;

        if (entries_cnt_per_core - i > chunk) {
            i += chunk ;
        } else {
            chunk = entries_cnt_per_core - i ;
            i = entries_cnt_per_core ;
        };

        thread_put += getFileContentNew_optimized_pointer(init, chunk, database) ;
    }

    //auto end = timeSinceEpochMillisecX();
    //std::cout << "[cost of second: " << (end - start)/1000.0  << ", # of PUTs: " << thread_put <<"]Thread: " << thread_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
    return thread_put ;
}

int replay_in_same_thread_wrapper_even(int thread_id,
                                       size_t offset,
                                       char *base,
                                       size_t entries_cnt_per_core,
                                       new_directs database,
                                       std::map<std::string, long> table_map) {
    std::cout << "Thread " << thread_id << " gets started, coreid: " << sched_getcpu() << ", offset: " << offset << std::endl;
    auto start = timeSinceEpochMillisecX();
    size_t thread_put = 0;
    std::unordered_set<long> all_files_names_temp={};
    for(auto &table : table_map) {
        all_files_names_temp.insert(table.second);
    }

    size_t chunk = 0;
    std::vector<one_container*> containers = {};
    for(size_t i=0; i < entries_cnt_per_core; ++i) {
        size_t rw = 0 ;
	    // take up ~40% execution time
        auto *entry = new one_container (base + offset + i * 27, rw);
        auto table_id = entry->table_id;
        if (all_files_names_temp.find (table_id) == all_files_names_temp.end ()) {
            std::cout << "thread_id = " << thread_id << ", Table id= " << table_id << " doesn't exist" << std::endl;
            continue;
        }
        containers.push_back(entry) ;
        chunk += 1;
        if (chunk == 512 || i == entries_cnt_per_core - 1) {
            thread_put += containers.size();
	        // take up ~60% execution time
            getFileContentNew_optimized(containers, database) ;
            chunk = 0;
		    for(auto &a:containers) delete a;
		    containers = {};
        }
    }

    auto end = timeSinceEpochMillisecX();
    std::cout << "[cost of second: " << (end - start)/1000.0  << ", # of PUTs: " << thread_put <<"]Thread: " << thread_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
    return 0 ;
}


bool is_file_exist(std::string fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

void _generate (std::pair<char *, int> &pair, std::vector<OneLog*>& vector,
	std::map<std::string, long> table_map) {
  size_t whole_len = pair.second;
  char *buffer = pair.first;
  size_t rw = 0;
  typedef unsigned long long int ULL_I;
  size_t ULL_LEN = sizeof (ULL_I);
  std::unordered_set<long> all_files_names_temp={};
  for(auto &table : table_map) {
	  all_files_names_temp.insert(table.second);
  }
  
  // optimized version
  /*
   |   cid   | <key,value,table_id,delete_true> | len |
   | 8 BYTES | 100*(8+8+2+1)                    | 8   |
   | <<<<<<<<<<<<     total 1916    >>>>>>>>>>>>>>>   |
   * */
  auto logGrpGenerator = new OneLogGroup(vector);
  while(rw < whole_len) {
    // read first 16 bytes
    ULL_I cid=0;
  	memcpy ((char *) &cid, buffer + rw, ULL_LEN);
	rw += ULL_LEN;
 
	ULL_I _len=0;
	memcpy ((char *) &_len, buffer + rw, ULL_LEN);
	rw += ULL_LEN;
 
	size_t stopPointer = rw+_len;
	logGrpGenerator->add_one_container (buffer, rw, _len, cid, stopPointer, all_files_names_temp);
  }
  vector = logGrpGenerator->GetItems ();
}
void replay_in_same_thread_object(xew_directs *db,
	char* array,
	size_t chunk_size,
	unsigned long long int cid,
	const std::unordered_set<long>& all_files_names_temp
	)
{
	getFileContentOz(db, array, cid, chunk_size, all_files_names_temp) ;
}

void replay_in_same_thread_objects(int thread_id,
	const std::vector<WrappedLog*>& workload_map,
	new_directs database,
	const std::unordered_set<long>& all_files_names_temp)
{

    //auto start = timeSinceEpochMillisecX();
    size_t thread_put = 0;
    auto iter = workload_map.begin();
	//std::cout << "Thread " << thread_id << " gets started, coreid: " << sched_getcpu()  << std::endl;
	for(;iter!=workload_map.end();iter++){
	  	int chunk_size = (*iter)->len/DECL_OPTIMAL_SIZE;
		thread_put += getFileContentNew_OneLogOptimized((*iter)->arr,(*iter)->cid, chunk_size , database, all_files_names_temp) ;
	}
     //	auto end = timeSinceEpochMillisecX();
    //std::cout << "[cost of second: " << (end - start)/1000.0  << ", # of PUTs: " << thread_put <<"]Thread: " << thread_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
}

void replay_in_shared_threads_table(
	SharedPoolTable &p,
	size_t workers,
	const char *buffer,
	size_t len,
	const std::unordered_set<long>& all_files_names_temp)
{
  	#ifdef SUBMIT_ALL_IN_ONE
  std::unordered_map<size_t,std::vector<WrappedLog*>> _intermediateLogs;
  typedef unsigned long long int ULL_I;
  size_t ULL_LEN = sizeof (ULL_I);
  ULL_I idx=0;
  size_t _count=0;
  while(idx < len){
	int _thread_id = _count % workers;
	// read first 16 bytes
	ULL_I cid=0;
	memcpy ((char *) &cid, buffer + idx, ULL_LEN);
	idx += ULL_LEN;

	ULL_I _len=0;
	memcpy ((char *) &_len, buffer + idx, ULL_LEN);
	idx += ULL_LEN;
	auto on_log = new WrappedLog();
	on_log->arr = (char *)buffer+idx;
	on_log->len = _len;
	on_log->cid = cid;
	
	_intermediateLogs[_thread_id].emplace_back (on_log);
	idx += _len;
	_count++;
  }
  // add all generated workload to threadpool for processing
  for (int i=0; i< workers; i++) {
	  p.doJob(std::bind (replay_in_same_thread_objects, i, _intermediateLogs[i], database, all_files_names_temp)) ;
  }
  p.waitDone() ;
#else
	typedef unsigned long long int ULL_I;
	size_t ULL_LEN = sizeof (ULL_I);
	ULL_I idx=0;
	size_t _count=0;
	while(idx < len){
	  int _thread_id = _count % workers;
	  // read first 16 bytes
	  ULL_I cid=0;
	  memcpy ((char *) &cid, buffer + idx, ULL_LEN);
	  idx += ULL_LEN;
  
	  ULL_I _len=0;
	  memcpy ((char *) &_len, buffer + idx, ULL_LEN);
	  idx += ULL_LEN;
	  p.doJob ([=] (size_t publisher_thread_id, xew_directs *db) {
	  	return replay_in_same_thread_object (db, (char *)buffer+idx, _len/DECL_OPTIMAL_SIZE, cid, all_files_names_temp);
	  });
	  idx += _len;
	  _count++;
	}
	p.waitDone() ;
#endif
}
void replay_in_shared_threads_opt(
	SharedThreadPool &p,
	size_t workers,
	const char *buffer,
	size_t len,
	const std::unordered_set<long>& all_files_names_temp,
	new_directs& database)
{
#ifdef SUBMIT_ALL_IN_ONE
  std::unordered_map<size_t,std::vector<WrappedLog*>> _intermediateLogs;
  typedef unsigned long long int ULL_I;
  size_t ULL_LEN = sizeof (ULL_I);
  ULL_I idx=0;
  size_t _count=0;
  while(idx < len){
	int _thread_id = _count % workers;
	// read first 16 bytes
	ULL_I cid=0;
	memcpy ((char *) &cid, buffer + idx, ULL_LEN);
	idx += ULL_LEN;

	ULL_I _len=0;
	memcpy ((char *) &_len, buffer + idx, ULL_LEN);
	idx += ULL_LEN;
	auto on_log = new WrappedLog();
	on_log->arr = (char *)buffer+idx;
	on_log->len = _len;
	on_log->cid = cid;
	
	_intermediateLogs[_thread_id].emplace_back (on_log);
	idx += _len;
	_count++;
  }
  // add all generated workload to threadpool for processing
  for (int i=0; i< workers; i++) {
	  p.doJob(std::bind (replay_in_same_thread_objects, i, _intermediateLogs[i], database, all_files_names_temp)) ;
  }
  p.waitDone() ;
#else
//	typedef unsigned long long int ULL_I;
//	size_t ULL_LEN = sizeof (ULL_I);
//	ULL_I idx=0;
//	size_t _count=0;
//	while(idx < len){
//	  int _thread_id = _count % workers;
//	  // read first 16 bytes
//	  ULL_I cid=0;
//	  memcpy ((char *) &cid, buffer + idx, ULL_LEN);
//	  idx += ULL_LEN;
//
//	  ULL_I _len=0;
//	  memcpy ((char *) &_len, buffer + idx, ULL_LEN);
//	  idx += ULL_LEN;
//	  p.doJob(std::bind (replay_in_same_thread_object, _thread_id, (char *)buffer+idx, _len/DECL_OPTIMAL_SIZE, cid, database, all_files_names_temp)) ;
//	  idx += _len;
//	  _count++;
//	}
//	p.waitDone() ;
#endif
}

void start_replay_for_all_files_direct_wo_threadpool(int file_count, std::string file_path, int replay_thr_count)
{
    std::vector<std::pair<char *,int>> logs;

    size_t total_bytes = 0 ;
    for(int fid=0;fid<file_count;fid++) {
        std::string path = file_path != "" ? file_path : LOG_FOLDER_DIR;
        std::string file_name = std::string(path) + \
                                std::string("Log-ThreadID-") + \
                                std::to_string(fid) + std::string(".txt");

        if (!is_file_exist(file_name)) {
            std::cout << "[ReplayDB] file does not exist: " << file_name << std::endl;
            continue;
        }

        size_t sz = get_file_size(file_name);
        int fd = open (file_name.c_str (), O_RDONLY);
        size_t ret = 0;
        void *ptr = NULL;
        char *buffer;

        ptr = (void *) malloc (sz);
        buffer = (char *) ptr;
        ret = read (fd, buffer, sz);
        if (ret == -1 || ret == 0) {
            std::cout << "[ReplayDB] file is empty " << ret << std::endl;
            continue;
        }
        total_bytes += ret ;
        logs.push_back(std::make_pair(buffer, ret)) ;
    }
	
    std::string info_path = file_path != ""? file_path + "/info": INFO_LOG_FOLDER_NAME ;
    std::string table_info_file_name = std::string(info_path) + "/Log-ThreadID-9999.txt";
    std::string counter_info_file_name = std::string(info_path) + "/Log-ThreadID-9888.txt";
    std::map<std::string, long> table_map = {};
    size_t _count=0;
    
    bool pre_process_okay = fileToMap(table_info_file_name,table_map);
    size_t info_trans = getTotalTransInfo(counter_info_file_name) ;
    if(!pre_process_okay)
        exit(1);
    
  std::unordered_set<long> all_files_names_temp={};
  for(auto &table : table_map) {
	  all_files_names_temp.insert(table.second);
  }
  
  void *ptr = NULL;
  char *buffer;
  ptr = (void *) malloc (total_bytes);
  buffer = (char *) ptr;
  size_t tmp = 0 ;
  for(auto it =logs.begin ();it != logs.end(); it++ ) {
	  memcpy(buffer + tmp, (*it).first, (*it).second) ;
	  tmp += (*it).second ;
  }

  // free the memory
  for(auto it =logs.begin ();it != logs.end(); it++ ) {
	  free((*it).first) ;
  }
  
  SharedPoolTable sharedThreads (replay_thr_count);
  sharedThreads.startAll ();
  auto start = std::chrono::steady_clock::now();

  // FIXME: check valid table
  replay_in_shared_threads_table(sharedThreads, replay_thr_count, buffer, total_bytes, all_files_names_temp) ;
  sharedThreads.genStats () ;
  sharedThreads.closeAll () ;

  auto end = std::chrono::steady_clock::now();
  auto diff = end - start;
  auto timeTaken = std::chrono::duration <long double, std::milli> (diff).count()  /1000.0 ;
  std::cout << "Time Taken : " << timeTaken*1000.0 << " millis\n";
  std::cout << "Replay Exec Spent " << timeTaken << " sec for txns: " << info_trans << std::endl;
  std::cout << "Throughput : " << info_trans / float(timeTaken)  << " ops/sec" << std::endl;
}

void start_replay_for_all_files_direct_wo(int file_count, std::string file_path, int replay_thr_count){
  std::vector<std::thread> threads(replay_thr_count);
  std::vector<std::pair<char *,int>> logs;
  size_t total_bytes = 0 ;

  for(int fid=0;fid<file_count;fid++) {
        std::string path = file_path != "" ? file_path : LOG_FOLDER_DIR;
        std::string file_name = std::string(path) + \
                                std::string("Log-ThreadID-") + \
                                std::to_string(fid) + std::string(".txt");

        if (!is_file_exist(file_name)) {
            std::cout << "[ReplayDB] file does not exist: " << file_name << std::endl;
            continue;
        }

        size_t sz = get_file_size(file_name);
        int fd = open (file_name.c_str (), O_RDONLY);
        size_t ret = 0;
        void *ptr = NULL;
        char *buffer;

        ptr = (void *) malloc (sz);
        buffer = (char *) ptr;
        ret = read (fd, buffer, sz);
        if (ret == -1 || ret == 0) {
            std::cout << "[ReplayDB] file is empty " << ret << std::endl;
            return;
        }
        total_bytes += ret ;
        logs.emplace_back(buffer, ret) ;
  }
  
  // vector of log --> vector<OneLogGroup>
//  std::vector<
  typedef unsigned long long int ULL_I;
  size_t ULL_LEN = sizeof (ULL_I);
  std::unordered_map<size_t,std::vector<WrappedLog*>> _intermediateLogs;
  std::string info_path = file_path != ""? file_path + "/info": INFO_LOG_FOLDER_NAME ;
  std::string table_info_file_name = std::string(info_path) + "/Log-ThreadID-9999.txt";
  std::string counter_info_file_name = std::string(info_path) + "/Log-ThreadID-9888.txt";
  std::map<std::string, long> table_map = {};
  bool pre_process_okay = fileToMap(table_info_file_name,table_map);
  size_t info_trans = getTotalTransInfo(counter_info_file_name) ;
  size_t _count=0;
  
  for(auto &each : logs) {
  	ULL_I idx=0;
  	char *buffer = each.first;
  	while(idx < each.second){
  	  int _thread_id = _count % replay_thr_count;
  	  // read first 16 bytes
  	  ULL_I cid=0;
  	  memcpy ((char *) &cid, buffer + idx, ULL_LEN);
  	  idx += ULL_LEN;
 
  	  ULL_I _len=0;
	  memcpy ((char *) &_len, buffer + idx, ULL_LEN);
	  idx += ULL_LEN;
	  auto on_log = new WrappedLog();
	  on_log->arr = buffer+idx;
	  on_log->len = _len;
	  on_log->cid = cid;
	  
	  _intermediateLogs[_thread_id].emplace_back (on_log);
	  idx += _len;
	  _count++;
  	}
  }
  
  std::unordered_set<long> all_files_names_temp={};
  for(auto &table : table_map) {
	  all_files_names_temp.insert(table.second);
  }
  
  // main DB ref
  // jay : it is okay for threads to create their own copy as per current silo setup
  new_directs database;
  
  auto start = std::chrono::steady_clock::now();
    for (int i=0; i< replay_thr_count; i++) {
        threads[i] = std::thread(replay_in_same_thread_objects, i,
        	                     _intermediateLogs[i],
        	                     database,
        	                     all_files_names_temp);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    for (auto &th : threads) {
        th.join();
    }
	auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    auto s = std::chrono::duration <long double, std::milli> (diff).count()  /1000.0;
    std::cout << "Time Taken : " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()<< " millis\n";
    std::cout << "Replay Exec Spent " << s << " sec for txns: " << info_trans << std::endl;
    std::cout << "Throughput : " << info_trans / float(s)  << " ops/sec" << std::endl;
}

double treplay_in_shared_threads(TSharedThreadPool &tp, size_t workers, char *log, size_t total_bytes){
  assert(total_bytes % 27 == 0) ;
    size_t entries_cnt_total = total_bytes / 27 ;

    size_t offset = 0 ;
    for (int i=0; i< workers; i++) {
        size_t entries_cnt_per_core = entries_cnt_total / workers ;
        if (i == workers - 1) {
            entries_cnt_per_core = entries_cnt_total - (workers - 1) * (entries_cnt_total / workers) ;
        }

        tp.doJob ([=] (int publisher_thread_id, ThreadDBWrapper *db) {
	  		Treplay_in_same_thread_wrapper_pointer_even(i,offset,log,entries_cnt_per_core,db->getDB ());
		});
        offset += entries_cnt_per_core * 27 ;
    }
    tp.waitDone() ;
    return 0 ;
}

unsigned long long int treplay_in_same_thread_opt_cid(char *buffer, size_t len) {
    typedef unsigned long long int ULL_I;
    size_t ULL_LEN = sizeof (ULL_I);
    ULL_I idx=0;

    ULL_I latest_commit_id = 0 ;

    while(idx < len){
        // read first 16 bytes
        memcpy ((char *) &latest_commit_id, buffer + idx, ULL_LEN);
        idx += ULL_LEN;

        ULL_I _len=0;
        memcpy ((char *) &_len, buffer + idx, ULL_LEN);
        idx += ULL_LEN;

        idx += _len;
    }

    return latest_commit_id;
}

size_t treplay_in_same_thread_opt(size_t par_id, char *buffer, size_t len, new_directs& database, const std::unordered_set<long>& all_files_names_temp) {
    //std::cout << "Thread " << par_id << " gets started, coreid: " << sched_getcpu()  << std::endl;
    //auto start = timeSinceEpochMillisecX();
    std::vector<WrappedLog*> _intermediateLogs;
    typedef unsigned long long int ULL_I;
    size_t ULL_LEN = sizeof (ULL_I);
    ULL_I idx=0;
    size_t _count=0;
    while(idx < len){
        // read first 16 bytes
        ULL_I cid=0;
        memcpy ((char *) &cid, buffer + idx, ULL_LEN);
        idx += ULL_LEN;

        ULL_I _len=0;
        memcpy ((char *) &_len, buffer + idx, ULL_LEN);
        idx += ULL_LEN;
        auto on_log = new WrappedLog();
        on_log->arr = (char *)buffer+idx;
        on_log->len = _len;
        on_log->cid = cid;

        _intermediateLogs.emplace_back (on_log);
        idx += _len;
        _count++;
    }

    size_t thread_put = 0;
    auto iter = _intermediateLogs.begin();
    for(;iter!=_intermediateLogs.end();iter++){
        int chunk_size = (*iter)->len/DECL_OPTIMAL_SIZE;
        thread_put += getFileContentNew_OneLogOptimized((*iter)->arr,(*iter)->cid, chunk_size , database, all_files_names_temp) ;
    }
    //auto end = timeSinceEpochMillisecX();
    //std::cout << "[cost of second: " << (end - start)/1000.0  << ", # of PUTs: " << thread_put <<"]Thread: " << par_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
    return thread_put;
}

size_t treplay_in_same_thread_opt_mbta(size_t par_id, char *buffer, size_t len, abstract_db* db, const std::unordered_set<long>& all_files_names_temp) {
    //std::cout << "Thread " << par_id << " gets started, coreid: " << sched_getcpu()  << std::endl;
    //auto start = timeSinceEpochMillisecX();
    std::vector<WrappedLog*> _intermediateLogs;
    typedef unsigned long long int ULL_I;
    size_t ULL_LEN = sizeof (ULL_I);
    ULL_I idx=0;
    size_t _count=0;
    while(idx < len){
        // read first 16 bytes
        ULL_I cid=0;
        memcpy ((char *) &cid, buffer + idx, ULL_LEN);
        idx += ULL_LEN;

        ULL_I _len=0;
        memcpy ((char *) &_len, buffer + idx, ULL_LEN);
        idx += ULL_LEN;
        auto on_log = new WrappedLog();
        on_log->arr = (char *)buffer+idx;
        on_log->len = _len;
        on_log->cid = cid;

        _intermediateLogs.emplace_back (on_log);
        idx += _len;
        _count++;
    }

    size_t thread_put = 0;
    auto iter = _intermediateLogs.begin();
    for(;iter!=_intermediateLogs.end();iter++){
        int chunk_size = (*iter)->len/DECL_OPTIMAL_SIZE;
        thread_put += getFileContentNew_OneLogOptimized_mbta((*iter)->arr,(*iter)->cid, chunk_size, db, all_files_names_temp) ;
    }
    //auto end = timeSinceEpochMillisecX();
    //std::cout << "[cost of second: " << (end - start)/1000.0  << ", # of PUTs: " << thread_put <<"]Thread: " << par_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
    return thread_put;
}

double replay_in_shared_threads(SharedThreadPool &p, size_t workers, char *log, size_t total_bytes, new_directs& database) {
    // divide the work evenly
    assert(total_bytes % 27 == 0) ;
    size_t entries_cnt_total = total_bytes / 27 ;

    size_t offset = 0 ;
    for (int i=0; i< workers; i++) {
        size_t entries_cnt_per_core = entries_cnt_total / workers ;
        if (i == workers - 1) {
            entries_cnt_per_core = entries_cnt_total - (workers - 1) * (entries_cnt_total / workers) ;
        }

        p.doJob(std::bind (replay_in_same_thread_wrapper_pointer_even, i, offset, log, entries_cnt_per_core, database)) ;
        offset += entries_cnt_per_core * 27 ;
    }
    p.waitDone() ;
    return 0 ;
}

double replay_in_multi_threads(char *log, size_t total_bytes, int replay_thr_count, new_directs& database,std::map<std::string, long>& table_map) {
    std::vector<std::thread> threads(replay_thr_count);
    auto ret = total_bytes;
    // std::cout << "total size of logs: " << ret / 1024.0/ 1024.0 << " MB, # of entries:" << ret / 27 << std::endl;

    // divide the work evenly
    assert(ret % 27 == 0) ;
    size_t entries_cnt_total = ret / 27 ;

    auto start = std::chrono::steady_clock::now();
    size_t offset = 0 ;
    for (int i=0; i< replay_thr_count; i++) {
        size_t entries_cnt_per_core = entries_cnt_total / replay_thr_count ;
        if (i == replay_thr_count - 1) {
            entries_cnt_per_core = entries_cnt_total - (replay_thr_count - 1) * (entries_cnt_total / replay_thr_count) ;
        }
        threads[i] = std::thread(replay_in_same_thread_wrapper_pointer_even, i, offset, log, entries_cnt_per_core, database);
        offset += entries_cnt_per_core * 27 ;
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    for (auto &th : threads) {
        th.join();
    }

    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    return std::chrono::duration <long double, std::milli> (diff).count()  /1000.0 ;
}


void start_replay_for_all_files_direct_even(int file_count, std::string file_path, int replay_thr_count,size_t max_buffer_allowed=21 * 512 * 1024) {
    std::vector<std::pair<char *,int>> logs;

    size_t total_bytes = 0 ;
    for(int fid=0;fid<file_count;fid++) {
        std::string path = file_path != "" ? file_path : LOG_FOLDER_DIR;
        std::string file_name = std::string(path) + \
                                std::string("Log-ThreadID-") + \
                                std::to_string(fid) + std::string(".txt");

        if (!is_file_exist(file_name)) {
            std::cout << "[ReplayDB] file does not exist: " << file_name << std::endl;
            continue;
        }

        size_t sz = get_file_size(file_name);
        int fd = open (file_name.c_str (), O_RDONLY);
        size_t ret = 0;
        void *ptr = NULL;
        char *buffer;

        ptr = (void *) malloc (sz);
        buffer = (char *) ptr;
        ret = read (fd, buffer, sz);
        if (ret == -1 || ret == 0) {
            std::cout << "[ReplayDB] file is empty " << ret << std::endl;
            continue;
        }
        total_bytes += ret ;
        logs.push_back(std::make_pair(buffer, ret)) ;
    }

    void *ptr = NULL;
    char *buffer;
    ptr = (void *) malloc (total_bytes);
    buffer = (char *) ptr;
    size_t tmp = 0 ;
    for(auto it =logs.begin ();it != logs.end(); it++ ) {
        memcpy(buffer + tmp, (*it).first, (*it).second) ;
        tmp += (*it).second ;
    }

    // free the memory
    for(auto it =logs.begin ();it != logs.end(); it++ ) {
        free((*it).first) ;
    }

    std::string info_path = file_path != ""? file_path + "/info": INFO_LOG_FOLDER_NAME ;
    std::string table_info_file_name = std::string(info_path) + "/Log-ThreadID-9999.txt";
    std::string counter_info_file_name = std::string(info_path) + "/Log-ThreadID-9888.txt";
    std::map<std::string, long> table_map = {};
    bool pre_process_okay = fileToMap(table_info_file_name,table_map);
    size_t info_trans = getTotalTransInfo(counter_info_file_name) ;
    if(!pre_process_okay)
        exit(1);
  #ifdef WEIHAI_IMPL  // discard this branch
  new_directs database;
  SharedThreadPool sharedThreads (replay_thr_count);
  auto start = std::chrono::steady_clock::now();

  // FIXME: check valid table
  replay_in_shared_threads(sharedThreads, replay_thr_count, buffer, total_bytes, database) ;
  sharedThreads.joinAll() ;
  #else
  auto start = std::chrono::steady_clock::now();
  TSharedThreadPool tpool (replay_thr_count);
  tpool.initPool (replay_thr_count);
  treplay_in_shared_threads(tpool, replay_thr_count, buffer, total_bytes) ;
  tpool.closeAll(replay_thr_count) ;
  // just to show how it spanned across threads
//  tpool.genStats ();
  #endif

  auto end = std::chrono::steady_clock::now();
  auto diff = end - start;
  auto timeTaken = std::chrono::duration <long double, std::milli> (diff).count()  /1000.0 ;
  std::cout << "Time Taken : " << timeTaken << " sec\n";
  std::cout << "Replay Exec Spent " << timeTaken << " sec for txns: " << info_trans << std::endl;
  std::cout << "Throughput : " << info_trans / float(timeTaken)  << " ops/sec" << std::endl;
}

void start_replay_for_all_files_direct(int file_count, std::string file_path, int replay_thr_count,size_t max_buffer_allowed=21 * 512 * 1024) {
  std::vector<std::pair<char *,int>> all_buffers;
  //std::vector<std::thread> threads;
  std::vector<std::thread> threads(replay_thr_count);

  // making assumption for log file size
  // vector is okay, but buffers must split up in large number of groups
  // code to print each file size

  for(int fid=0;fid<file_count;fid++) {
    std::string path = file_path != ""? file_path: LOG_FOLDER_DIR ;
	std::string file_name = std::string (path) + \
                                std::string ("Log-ThreadID-") + \
                                std::to_string (fid) + std::string (".txt");

    if (!is_file_exist(file_name)) {
        std::cout << "[ReplayDB] file does not exist: " << file_name << std::endl;
	    continue ;
	}

	size_t sz = get_file_size (file_name);
    // JAY : making last line irrelevant
	size_t original_sz = sz;
	size_t covered = 0;

	size_t total_entries_local = 0;
	std::set<unsigned long long int> unique_keys;
	int fd = open (file_name.c_str (), O_RDONLY);

	size_t ret = 0;
	bool break_just_after=false;
	void *ptr = NULL;
	char *buffer;
	size_t left = 0;

	ptr = (void *) malloc (sz);
	buffer = (char *) ptr;
	ret = read (fd, buffer, sz);
	if (ret == -1 || ret == 0) {
        std::cout << "[ReplayDB] file is empty " << ret << std::endl;
        continue;
	}
	left = ret;
	size_t offset=0;
//	max_buffer_allowed = std::min(max_buffer_allowed,ret);

	size_t slices = ret/max_buffer_allowed;
	std::cout << "total slices: " << slices << ", ret:" << ret << ", max_buffer_allowed: " << max_buffer_allowed << std::endl;
	size_t extra = ret%max_buffer_allowed;

	for (size_t i = 0; i < slices; ++i) {
	  size_t pick = max_buffer_allowed;
	  //std::cout << "Start processing file : " << std::to_string(i) << ", " << file_name << " Total Size : " << pick << " Entries: " << pick / 21 << std::endl;
	  //std::cout << "File Size(in MB) : " << pick / 1024.0 / 1024.0 << " MB ==> in KB : " << pick / 1024.0 << std::endl;
	  all_buffers.push_back (std::make_pair (buffer+offset, pick));
	  offset+=pick;
	}
	if(extra){
	  int magic = extra/27*27;
	  extra = magic;
	  //std::cout << "Start processing file : " << file_name << " Total Size : " << extra << " Entries: " << extra / 21 << std::endl;
	  //std::cout << "File Size(in MB) : " << extra / 1024.0 / 1024.0 << " MB ==> in KB : " << extra / 1024.0 << std::endl;
	  all_buffers.push_back (std::make_pair (buffer+offset, extra));
	}
  }
  //std::cout << "\nprinting all buffer size : " << all_buffers.size() << std::endl;

//  std::vector<long> table_names={10001,10002,10003,10004,10005,10006,10007,10008,10009,10010,10011,10012};
    std::string path = file_path != ""? file_path + "/info": INFO_LOG_FOLDER_NAME ;
  	std::string table_info_file_name = std::string(path) + "/Log-ThreadID-9999.txt";
	std::string counter_info_file_name = std::string(path) + "/Log-ThreadID-9888.txt";
	std::map<std::string, long> table_map = {};
	bool pre_process_okay = fileToMap(table_info_file_name,table_map);
	size_t info_trans = getTotalTransInfo(counter_info_file_name) ;
	if(!pre_process_okay)
	  exit(1);

  // init a database
  new_directs database;
//  for(auto &table_id: table_names){
//      actual_ds hh;
//      hh.set_table_name (table_id);
//      database[table_id] = hh;
//  }
  size_t txns=0;

  long double spent_time=0.0;
  // Replay instance
  // may be useful in detached thread pool design
  //auto r = new ReplayRunner();
  //r->initialize ();
//  ProfilerStart("abc");
    std::cout << "nums of threads: " << std::to_string(replay_thr_count) << std::endl ;
    std::cout << "size of each chunk : " << max_buffer_allowed / 1024.0 / 1024.0 << " MB ==> in KB : " << max_buffer_allowed / 1024.0 << std::endl;
    std::cout << "size of chunks (all_buffers): " << std::to_string(all_buffers.size()) << std::endl ;
    auto rng = std::default_random_engine {};
    // shuffle all_buffers
    // std::shuffle(all_buffers.begin(), all_buffers.end(), rng);
    // std::cout << "size of chunks (all_buffers) - after shuffle: " << std::to_string(all_buffers.size()) << std::endl ;
    auto start = std::chrono::steady_clock::now();
    int tmp = 0, stride = all_buffers.size() / replay_thr_count ;

    for (int i=0; i<replay_thr_count; i++) {
      	int offset = tmp;
      	bool end_true = false;
        if (i == replay_thr_count - 1) {
            end_true = true;
        }

	//threads.emplace_back(std::thread(replay_in_same_thread_wrapper, i, offset, all_buffers, end_true, stride, database, table_map));

        threads[i] = std::thread(replay_in_same_thread_wrapper, i, offset, all_buffers, end_true, stride, database, table_map); 

        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }

        tmp += stride ;
    }

    for (auto &th : threads) {
        th.join();
    }

//  for(auto &each : all_buffers) {
////	txns+=replay_process_stat (each.first, each.second, std::ref (database), replay_thr_count, table_map,spent_time);
//	replay_in_same_thread_direct(each.first, each.second, std::ref (database),table_map, 2);
//  }
//  ProfilerStop();
//  spent_time = ;
  
  auto end = std::chrono::steady_clock::now();
  auto diff = end - start;
  auto s = std::chrono::duration <long double, std::milli> (diff).count()  /1000.0;
  std::cout << "Time Taken : " << std::chrono::duration_cast<std::chrono::milliseconds>(diff).count()<< " millis\n";

  std::cout << "Replay Exec Spent " << s << " sec for txns : " << txns_count << std::endl;
  std::cout << "Replay Exec Spent " << s << " sec for txns - info: " << info_trans << std::endl;
  std::cout << "# of PUTs: " << put_count  << std::endl;
  std::cout << "Throughput : " << txns_count / float(s)  << " ops/sec" << std::endl;
  std::cout << "Throughput - info : " << info_trans / float(s)  << " ops/sec" << std::endl;
}

void replay_in_same_thread_objects_mbta(int thread_id,
                                        const std::vector<WrappedLogV2*>& workload_map,
                                        abstract_db *db) {
    auto start = timeSinceEpochMillisecX();
    TThread::set_id(thread_id);
    actual_directs::thread_init();

    size_t thread_put = 0;
    auto iter = workload_map.begin();
    for(;iter!=workload_map.end();iter++){
        thread_put += getFileContentNew_OneLogOptimized_mbta_v2((*iter)->arr,
                                                                (*iter)->cid,
                                                                (*iter)->count,
                                                                (*iter)->len,
                                                                db) ;
    }
    auto end = timeSinceEpochMillisecX();
    std::cout << "[cost of second: " << (end - start)/1000.0  << ", # of PUTs: " << thread_put <<"]Thread: " << thread_id << ", coreid: " << sched_getcpu() << " is completed!" << std::endl;
}


void start_replay_for_all_files_direct_mbta(int file_count, std::string file_path, int replay_thr_count, abstract_db *db){
    std::vector<std::thread> threads(replay_thr_count);
    std::vector<std::pair<char *,int>> logs;
    size_t total_bytes = 0 ;

    for(int fid=0;fid<file_count;fid++) {
        std::string path = file_path != "" ? file_path : LOG_FOLDER_DIR;
        std::string file_name = std::string(path) + \
                                std::string("Log-ThreadID-") + \
                                std::to_string(fid) + std::string(".txt");

        if (!is_file_exist(file_name)) {
            std::cout << "[ReplayDB] file does not exist: " << file_name << std::endl;
            continue;
        }
        std::cout << "reading file: " << file_name << std::endl;

        size_t sz = get_file_size(file_name);
        int fd = open (file_name.c_str (), O_RDONLY);
        size_t ret = 0;
        void *ptr = NULL;
        char *buffer;

        ptr = (void *) malloc (sz);
        buffer = (char *) ptr;
        ret = read (fd, buffer, sz);
        if (ret == -1 || ret == 0) {
            std::cout << "[ReplayDB] file is empty " << ret << std::endl;
            return;
        }
        total_bytes += ret ;
        logs.emplace_back(buffer, ret) ;
    }

    typedef unsigned long long int ULL_I;
    size_t ULL_LEN = sizeof (ULL_I);
    std::unordered_map<size_t,std::vector<WrappedLogV2*>> _intermediateLogs;
    std::string info_path = file_path != ""? file_path + "/info": INFO_LOG_FOLDER_NAME ;
    std::string table_info_file_name = std::string(info_path) + "/Log-ThreadID-9999.txt";
    std::string counter_info_file_name = std::string(info_path) + "/Log-ThreadID-9888.txt";
    std::map<std::string, long> table_map = {};
    bool pre_process_okay = fileToMap(table_info_file_name,table_map);
    size_t info_trans = getTotalTransInfo(counter_info_file_name) ;
    size_t nums=0;

    for(auto &each : logs) {
        ULL_I idx=0;
        char *buffer = each.first;
        std::cout << "total len: " << each.second << std::endl;
        while(idx < each.second) {
            int _thread_id = nums % replay_thr_count;

            ULL_I cid=0;
            memcpy ((char *) &cid, buffer + idx, ULL_LEN);
            idx += ULL_LEN;

            // 2. get count of K-V
            unsigned short int count=0;
            memcpy ((char *) &count, buffer + idx, sizeof(unsigned short int));
            idx += sizeof(unsigned short int) ;

            // 3. get len of K-V
            unsigned int _len=0;
            memcpy ((char *) &_len, buffer + idx, sizeof(unsigned int));
            idx += sizeof(unsigned int) ;

            // 4. wrap K-V
            auto on_log = new WrappedLogV2() ;
            on_log->cid = cid;
            on_log->count = count;
            on_log->len = _len;
            on_log->arr = (char *)buffer + idx;

            // 5. skip K-V pairs
            idx += _len ;

            _intermediateLogs[_thread_id].emplace_back (on_log);
            nums++;
        }
    }

    for(auto &table : table_map) {
        // init all table
        db->open_index(table.second) ;
    }

    //auto start = std::chrono::steady_clock::now();
    for (int i=0; i< replay_thr_count; i++) {
        threads[i] = std::thread(replay_in_same_thread_objects_mbta,
                                 i,
                                 _intermediateLogs[i],
                                 db);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    double tTotal = 0.0;
    auto start = std::chrono::steady_clock::now();
    for (auto &th : threads) {
        th.join();
        auto end = std::chrono::steady_clock::now();
        auto diff = end - start;
        auto s = std::chrono::duration <long double, std::milli> (diff).count()  /1000.0;
        tTotal += s ;
    }

    std::cout << "Time Taken : " << 1000 * tTotal / replay_thr_count << " millis\n";
    std::cout << "Replay Exec Spent " << tTotal / replay_thr_count << " sec for txns: " << info_trans << std::endl;
    std::cout << "Throughput : " << info_trans / float(tTotal / replay_thr_count)  << " ops/sec" << std::endl;
}

unsigned long long int get_latest_commit_id(char *buffer, size_t len) {
    assert(len >= 8) ;
    unsigned long long int _last_commit_id = 0;
    memcpy((char *)&_last_commit_id, buffer + len - 8, 8) ;
    return _last_commit_id;
}

size_t treplay_in_same_thread_opt_mbta_v2(size_t par_id, char *buffer, size_t len, abstract_db* db) {
    assert(len >= 8) ;
    len -= 8; // eliminate last 8 bytes
    typedef unsigned long long int ULL_I;
    size_t ULL_LEN = sizeof (ULL_I);

    std::vector<WrappedLogV2*> _intermediateLogs;

    size_t idx=0;
    while (idx < len) {
        // 1. get cid
        ULL_I cid=0;
        memcpy ((char *) &cid, buffer + idx, ULL_LEN);
        idx += ULL_LEN;

        // 2. get count of K-V
        unsigned short int count=0;
        memcpy ((char *) &count, buffer + idx, sizeof(unsigned short int));
        idx += sizeof(unsigned short int) ;

        // 3. get len of K-V
        unsigned int _len=0;
        memcpy ((char *) &_len, buffer + idx, sizeof(unsigned int));
        idx += sizeof(unsigned int) ;

        // 4. wrap K-V
        auto on_log = new WrappedLogV2() ;
        on_log->cid = cid;
        on_log->count = count;
        on_log->len = _len;
        on_log->arr = (char *)buffer + idx;

        // 5. skip K-V pairs
        idx += _len ;

        _intermediateLogs.emplace_back (on_log);
    }

    size_t thread_put = 0;
    auto iter = _intermediateLogs.begin();
    for (;iter!=_intermediateLogs.end();iter++) {
        thread_put += getFileContentNew_OneLogOptimized_mbta_v2((*iter)->arr,
                                                                (*iter)->cid,
                                                                (*iter)->count,
                                                                (*iter)->len,
                                                                db) ;
    }
    return thread_put ;
}

size_t treplay_in_same_thread_scanned_logs(size_t par_id, char *buffer, size_t len, abstract_db* db) {
    str_arena arena;
    void *buf = NULL ;

    int pid = rand() ;
    int prev = 0;
    size_t cur = 0;
    int thread_put = 0;
    unsigned short int table_id;
    int key_len, value_len;
    std::string key = "", key_len_str = "";
    std::string value = "", value_len_str = "";

    //std::cout << "#######[1]cur:" << par_id << ", " << cur << ", len:" << len << ", pid=" << pid << std::endl;
    while (cur < len) {
        // 1. get the table_id
        //std::cout << "[2]cur:" << par_id << ", " << cur << ", len:" << len << ", pid=" << pid << std::endl;
        while (*(buffer + cur) != '|') cur ++;

        //std::cout << "[3]cur:" << par_id << ", " << cur << ", len:" << len << ", pid=" << pid << std::endl;
        std::string str_table = std::string(buffer+prev, cur-prev);
        table_id = (unsigned short) strtoul(str_table.c_str(), NULL, 0);
        prev = cur + 1;
        cur = prev ;
        //std::cout << "[4]cur:" << par_id << ", " << cur << ", len:" << len << ", pid=" << pid << std::endl;

        // 2. get the size of key
        while (*(buffer + cur) != '|') cur ++;
        key_len_str = std::string(buffer+prev, cur-prev);
        key_len = (unsigned short) strtoul(key_len_str.c_str(), NULL, 0);
        prev = cur + 1;
        cur = prev ;
        //std::cout << "[5]cur:" << par_id << ", " << cur << ", len:" << len << ", key_len_str=" << key_len_str << ", pid=" << pid << std::endl;

        // 3. get the size of value
        while (*(buffer + cur) != '|') cur ++;
        value_len_str = std::string(buffer+prev, cur-prev);
        value_len = (unsigned short) strtoul(value_len_str.c_str(), NULL, 0);
        prev = cur + 1;
        cur = prev ;
        //std::cout << "[6]cur:" << par_id << ", " << cur << ", len:" << len << ", value_len_str=" << value_len_str << ", pid=" << pid << std::endl;

        // 4. get the key
        key = std::string(buffer+cur, key_len);
        cur += key_len;
        prev = cur;
        //std::cout << "[7]cur:" << par_id << ", " << cur << ", len:" << len << ", pid=" << pid << std::endl;

        // 5. get the value
        value = std::string(buffer+cur, value_len);
        cur += value_len;
        prev = cur;
        //std::cout << "[8]cur:" << par_id << ", " << cur << ", len:" << len << ", pid=" << pid << std::endl;

        //std::cout << "table_id: " << table_id << ", # of key: " << key_len << ", len of value: " << value_len << ", key: " << key << ", value: " << value << std::endl;
        //std::cout << "table_id: " << table_id << ", # of key: " << key_len << ", len of value: " << value_len << std::endl;

        if (table_id > 20000 || table_id < 10000) {
            std::cout << "table_id[ReplayDB.cc]: " << table_id << std::endl;
            //std::cout << "str_table=" << str_table << ", len=" << len << ", cur=" << cur << ", # of key: " << key_len << ", len of value: " << value_len << std::endl;
            continue ;
        }

        thread_put ++;
        int try_cnt = 1 ;
        while (1) {
            try {
                void *txn = db->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
                abstract_ordered_index *table_index = db->open_index(table_id) ;
                table_index->put(txn, key, value);
                auto ret = db->commit_txn_no_paxos(txn);
                //if (ret != 1)
                //    std::cout << "[treplay_in_same_thread_scanned_logs]error key=" << key << ", value=" << value << ", status=" << ret << std::endl;
                if (try_cnt > 1) {
                    std::cout << "succeed at retry#:" << try_cnt << std::endl;
                }
                break ;
            } catch (...) {   // if abort happens, replay it until it succeeds
                std::cout << "exception, retry#:" << try_cnt << std::endl;
                try_cnt += 1 ;
            }
        }
    }  // end of while loop
    return thread_put;
}

#ifndef MASS_DIRECT

void start_replay_for_all_files(int file_count,int replay_thr_count,size_t max_buffer_allowed=48 * 1024) {
  std::vector<std::pair<char *,int>> all_buffers;
  std::vector<std::thread> threads;

  // making assumption for log file size
  // vector is okay, but buffers must split up in large number of groups
  // code to print each file size

  for(int fid=0;fid<file_count;fid++) {
    std::string path = "/home/weihshen/Desktop/silo-sto/prev_logs/" ;
    // LOG_FOLDER_DIR
	std::string file_name = std::string (path) + \
                                std::string ("Log-ThreadID-") + \
                                std::to_string (fid) + std::string (".txt");

	size_t sz = get_file_size (file_name);
	size_t original_sz = sz;
	size_t covered = 0;

	size_t total_entries_local = 0;
	std::set<unsigned long long int> unique_keys;
	int fd = open (file_name.c_str (), O_RDONLY);

	size_t ret = 0;
	bool break_just_after=false;
	void *ptr = NULL;
	char *buffer;
	size_t left = 0;

	ptr = (void *) malloc (sz);
	buffer = (char *) ptr;
	ret = read (fd, buffer, sz);
	left = ret;
	size_t offset=0;
//	max_buffer_allowed = std::min(max_buffer_allowed,ret);

	size_t slices = ret/max_buffer_allowed;
	size_t extra = ret%max_buffer_allowed;

	for (size_t i = 0; i < slices; ++i) {
	  size_t pick = max_buffer_allowed;
	  std::cout << "Start processing file : " << file_name << " Total Size : " << pick << " Entries: " << pick / 48 << std::endl;
	  std::cout << "File Size(in MB) : " << pick / 1024.0 / 1024.0 << " MB ==> in KB : " << pick / 1024.0 << std::endl;
	  all_buffers.push_back (std::make_pair (buffer+offset, pick));
	  offset+=pick;
	}
	if(extra){
	  std::cout << "Start processing file : " << file_name << " Total Size : " << extra << " Entries: " << extra / 48 << std::endl;
	  std::cout << "File Size(in MB) : " << extra / 1024.0 / 1024.0 << " MB ==> in KB : " << extra / 1024.0 << std::endl;
	  all_buffers.push_back (std::make_pair (buffer+offset, extra));
	}
  }
  //std::cout << "\nprinting all buffer size : " << all_buffers.size() << std::endl;

  std::vector<long> table_names={10001,10002,10003,10004,10005,10006,10007,10008,10009,10010,10011,10012};
    std::string path2 = "/home/weihshen/Desktop/silo-sto/prev_logs/info" ;
    // INFO_LOG_FOLDER_NAME
  std::string table_info_file_name = std::string(path2) + "/Log-ThreadID-9999.txt";
	STd::map<std::string, long> table_map = {};
	bool pre_process_okay = fileToMap(table_info_file_name,table_map);
	if(!pre_process_okay)
	  exit(1);

  // init a database
  new_ds database;
//  for(auto &table_id: table_names){
//      actual_ds hh;
//      hh.set_table_name (table_id);
//      database[table_id] = hh;
//  }
  size_t txns=0;

  long double spent_time=0.0;
  // Replay instance
  // may be useful in detached thread pool design
  //auto r = new ReplayRunner();
  //r->initialize ();
  ProfilerStart("abc");

  // detached, thread_local
  std::cout << "nums of threads: " << std::to_string(replay_thr_count) << std::endl ;
  int tmp = 0, stride = all_buffers.size() / replay_thr_count ;

  for (int i=0; i<replay_thr_count; i++) {
      std::vector<std::pair<char *,int>> chunks ;
      if (i == replay_thr_count - 1) {
          chunks = std::vector<std::pair<char *,int>>(all_buffers.begin() + tmp, all_buffers.end()) ;
      } else {
          chunks = std::vector<std::pair<char *,int>>(all_buffers.begin() + tmp, all_buffers.begin() + tmp + stride) ;
      }
      tmp += stride ;
      threads.push_back(std::thread(replay_in_same_thread_wrapper, i, chunks, database, table_map));
  }

  for (auto &th : threads) {
      th.join();
  }

//  for(auto &each : all_buffers) {
////	txns+=replay_process_stat (each.first, each.second, std::ref (database), replay_thr_count, table_map,spent_time);
//	replay_in_same_thread(each.first, each.second, std::ref (database),table_map, 0);
//  }
  ProfilerStop();
  auto s = spent_time/1000.0/1000.0/1000.0;
  std::cout << "Spent " << s << " sec" << std::endl;
  std::cout << "Throughput : " << 878918 / s  << " ops/sec" << std::endl;
}

void replay_container(char *log, size_t len, int nthreads,std::map<std::string, long>& table_map, IndependentThread* processor){
  #if 0
  	static bool once = [](){
		{
		  Transaction::epoch_advance_callback = [] (unsigned) {
            // just advance blindly because of the way Masstree uses epochs
            globalepoch++;
	};
	  // someone has to do this (they don't provide us with a general init callback)
	  std::cout << "static_init done once" << std::endl;
	  actual_ds::static_init();
	  // need this too
	  pthread_t advancer;
	  pthread_create(&advancer, NULL, Transaction::epoch_advancer, NULL);
	  pthread_detach(advancer);
		}
	  return true;
	} ();
  #endif
  	
  
  if(len <=0){
	std::cout << "Empty Strings : " << std::endl;
	return;
  }
  
  int wthreads=1;
  // TODO database init
	
  #if 1
	
	//TODO files_names
	std::vector<container_hello*> files_names = {};
//	gettimeofday(&tv1, NULL);
	load_objects_from_buffer(log, len,wthreads,nthreads,table_map,files_names);
	
	auto start = std::chrono::steady_clock::now();
	
//	std::cout << "[DEBUG] adding " << files_names.size() << " jobs" << std::endl;
	
    for (auto &a:files_names) {
//      std::cout << a->GetTableID () << " :: " << a->GetContainerCounts () << " :: " << a->GetThreadID () << std::endl;
	  add_new_work(processor,a, a->GetTableID ());
	}
 
 	#if 1
  	static bool once = [](){
		{
		  pthread_t advancer;
		  pthread_create(&advancer, NULL, IndependentThread::wait_for_finish, NULL);
		  pthread_detach(advancer);
		}
	  return true;
	} ();
  #endif
 	
//    IndependentThread::wait_for_finish();
//    auto end = std::chrono::steady_clock::now();
    
//    auto diff = end - start;
//    std::cout << std::endl << " ||||| <<<< "<< std::chrono::duration <double, std::milli> (diff).count() << " ms" << std::endl;
    
//    std::cout << "Finished processing across" << std::endl;
//  	do_verify(table_map, database, files_names);
    unsigned long long int txns_total=0;
    
    // cleanup
//    for(auto &a:files_names){
//      txns_total+=a->GetContainerCounts ();
//      a->free_all ();
//      delete a;
//    }
//    all_txns_recorded.emplace_back (files_names);
//    std::cout << "Txns : " << txns_total << std::endl;
  #endif
//    std::cout << "Finished" << std::endl;
}

void start_replay_container_processing(int file_count,int replay_thr_count,IndependentThread* processor){
  std::string file_name = std::string (LOG_FOLDER_DIR) + \
								std::string ("Log-ThreadID-") + \
								std::to_string (file_count) +std::string(".txt" );
  std::cout << "Start processing file : " << file_name << std::endl;
  size_t sz = get_file_size(file_name);
  size_t original_sz = sz;
  size_t covered = 0;
  void *ptr=NULL;
  char *buffer;
  size_t rw=0;
  size_t ret=0;
  size_t total_entries_local = 0;
  std::set<unsigned long long int> unique_keys;
  int fd = open(file_name.c_str(), O_RDONLY);
  
  ptr = (void *) malloc (sz);
  buffer = (char *) ptr;
  ret = read (fd, buffer, sz);
  
  std::vector<long> table_names={10001,10002,10003,10004,10005,10006,10007,10008,10009,10010,10011,10012};
  	std::string table_info_file_name = std::string(INFO_LOG_FOLDER_NAME) + "/Log-ThreadID-9999.txt";
	std::map<std::string, long> table_map = {};
	bool pre_process_okay = fileToMap(table_info_file_name,table_map);
	if(!pre_process_okay)
	  exit(1);
	

  replay_container(buffer, ret, replay_thr_count,table_map, processor);
}
#endif

#if 0
int main(){
//  start_replay_processing(0,1);
//
  std::map<int,MassTreeWrapper*> db_objs;
  std::vector<long> table_names={10001,10002,10003,10004,10005,10006,10007,10008,10009,10010,10011,10012};
  
  for(auto &table_id : table_names) {
      actual_ds h;
  	  MassTreeWrapper* one_obj = new MassTreeWrapper(h, table_id);
  	  db_objs[table_id] =  one_obj;
  }
  
  auto tt = new IndependentThread(MAX_THREAD_COUNT);
  tt->start_all (db_objs);
  
  
  // TODO add work here
  start_replay_container_processing (9, MAX_THREAD_COUNT, tt);
  
  return 0;
}
#endif
