//
// Created by mrityunjay kumar on 2019-07-13.
//


#pragma once

#include <string>
#include <iostream>
#include <mutex>
#include <functional>
#include <thread>
#include <vector>
#include <queue>
#include <sstream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>
#include <exception>
#include <fstream>
#include <vector>
#include <condition_variable>
#include <glob.h>
#include <sys/time.h>
#include <future>
#include <shared_mutex>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>
#include <set>
#include <unordered_set>

#include "../abstract_db.h"
#include "function_pool.h"
//#include "OutputDataSerializer.h"

#define  MAXLINE 100

#define MAC 0



#if MAC
#define LOG_FOLDER_DIR "/Users/mrityunjaykumar/CLionProjects/Dummy523/log/"
#else
#ifdef MASS_DIRECT
#ifdef TEST_REPLAY_SPEED_FEAT
  #ifndef LOG_FOLDER
  		#define LOG_FOLDER "/home/jay/prev_logs/"
  #endif
#elif TEST_WRITE_SPEED_FEAT
  #ifndef LOG_FOLDER
  		#define LOG_FOLDER "/home/jay/prev_logs/"
  #endif
#else
  #ifndef LOG_FOLDER
  		#define LOG_FOLDER "/home/AzureUser/prev_logs/"
  #endif
#endif

#ifdef LOG_FOLDER
	#define LOG_FOLDER_NAME LOG_FOLDER
#endif

#define INFO_SUFFIX "info/"
#define R_SUFFIX "replay_logs/"
#define W_SUFFIX "write_logs/"

#define STR3(STR1) STR1 INFO_SUFFIX
#define STR2(STR1) STR1 R_SUFFIX
#define STR4(STR1) STR1 W_SUFFIX

#define INFO_LOG_FOLDER_NAME STR3(LOG_FOLDER)
#define REPLAY_SAVER_PATH STR2(LOG_FOLDER)
#define WRITE_STATS_SAVER_PATH STR4(LOG_FOLDER)
#endif
#define LOG_FOLDER_DIR LOG_FOLDER_NAME
#endif

size_t getFileContentNew_OneLogOptimized_mbta_v2(char *buffer, unsigned long long int cid, unsigned short int count, unsigned int len, abstract_db* db); // 05/05/2021 weihshen
size_t getFileContentNew_OneLogOptimized(char *buffer,
	unsigned long long int cid, size_t chunk, new_directs &h,const std::unordered_set<long>& all_files_names_temp);
size_t getFileContentNew_OneLogOptimized_mbta(char *buffer,
                                         unsigned long long int cid, size_t chunk, abstract_db* db, const std::unordered_set<long>& all_files_names_temp);
size_t load_objects_from_buffer_no_table(char *buffer, int length,int wthreads,int nthreads, const std::map<std::string,
	long>& table_map,std::vector<one_container*>& all_containers);

void load_objects_from_buffer(char *buffer, int length,int wthreads,int nthreads, const std::map<std::string,
	long>& table_map,std::vector<container_hello*>& all_files_names);

bool fileToMap(const std::string &filename,std::map<std::string,long> &fileMap);
size_t getTotalTransInfo(const std::string &filename);
std::vector<std::string> GetDecoded(std::string input);
#ifndef MASS_DIRECT
bool getFileContent(const container_hello* arg, ds &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed);

bool getFileContentNew(const container_hello* arg, new_ds &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed);

bool getFileContentNew_t(const std::vector<one_container*>& arg, new_ds &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed);
#endif
size_t getFileContentNew_optimized(const std::vector<one_container*>& arg, new_directs &h);
size_t getFileContentNew_optimized_pointer(char *init, size_t chunk, new_directs &h) ;
void insertEntry(one_container* entry, new_directs &h);
void insertEntry_pointer(unsigned long long int* cid,
                         unsigned long long int* k,
                         unsigned long long int* v,
                         unsigned short int* table_id,
                         unsigned char* delete_true,
                         new_directs &h) ;
bool getFileContentNew_direct(const std::vector<one_container*>& arg, new_directs &h, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed,
	std::atomic<size_t >& txns_count_this);

long get_file_size(std::string filename);

typedef new_directs xaw_directs;

// https://stackoverflow.com/questions/26516683/reusing-thread-in-loop-c
class NewDBWrapper{
  xaw_directs db;
  size_t thread_id;
 public:
  NewDBWrapper()= delete;
  NewDBWrapper(size_t thread_id){
    this->db = {};
    this->thread_id = thread_id;
    std::cout << "DB Created with wrapper thread id : " << thread_id << " :::addr : " << &(this->db) << std::endl;
  }
  void insert(const long& table_id,
  	          const std::string& key,
  	          const std::string& value,
  	          bool(*compar)(const std::string& newValue,const std::string& oldValue))
  {
    this->db[table_id].put (key, value, compar);
  }
  void insert(const long& table_id,
  	          const size_t& iter,
  	          const size_t& t,
  	          const std::string& data,bool(*compar)(const std::string& newValue,const std::string& oldValue))
  {
	if(this->thread_id != t){
	  // NOT A BUG
	  // this is not a bug since payload can be run by any worker threads
	  // just we had to make sure that each thread has consistent table id maps
	}
	std::string key = "123"+std::to_string(iter);
    this->db[table_id].put (key, data, compar);
  }
  xaw_directs& getDB(){
    return this->db;
  }
};
typedef NewDBWrapper xew_directs;

size_t getFileContentOz(xew_directs *db,
	char *buffer,
	size_t chunk,
	unsigned long long int cid,
	const std::unordered_set<long>& all_files_names_temp);

class SharedPoolTable
{
 private:
  std::unordered_map<size_t,xew_directs*> providerMap;
public:

    explicit SharedPoolTable (size_t threads) : shutdown_ (false), _prod_ready(false)
    {
        // Create the specified number of threads
        for (size_t i = 0; i < threads; ++i) {
		  if (this->providerMap.find (i) == this->providerMap.end ()) {
			this->providerMap[i] = new xew_directs (i);
		  }
		}
        for (size_t i = 0; i < threads; ++i) {
		  threads_.emplace_back (std::thread ([this,i] () {
		    this->threadEntry (i);
		  }));
		}
    }

    ~SharedPoolTable () = default;

    void startAll() {
		{
			  // Unblock any threads and tell them to stop
			  std::unique_lock <std::mutex> l (lock_);
  
			  _prod_ready = true;
			  condVar_.notify_all();
		}
    }
    
    void closeAll() {
      {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);
            shutdown_ = true;
            condVar_.notify_all();
	  }
	  for (auto& thread : threads_)
            thread.join ();
    }

    void doJob (std::function <void (size_t,xew_directs *)> func)
    {
        tasks ++ ;
        // Place a job on the queue and unblock a thread
        std::unique_lock <std::mutex> l (lock_);

        jobs_.emplace (std::move (func));
        condVar_.notify_one();
    }

    size_t getNotRunningSize() {
        return jobs_.size() ;
    }

    void waitDone() {
        while (tasks != done) {};
        done = 0 ;
        tasks = 0 ;
    }
    
    void genStats(bool clear=false) {
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
      std::cout << "------------------------------------------\n";
      
      size_t _count =0;
      for(auto &each : _cnt){
          std::cout << static_cast<int>(each.first) << " , " << static_cast<int>(each.second) << std::endl;
          _count+=each.second;
	  }
	  std::cout << "------------------------------------------\n";
	  std::cout << "------------------------------------------\n";
	  if(clear)
	  	_cnt = {};
      std::cout << " Total job done : " << static_cast<int>(_count) << std::endl;
      std::cout << "------------------------------------------\n";
    }

    size_t getDone() {
        return done ;
    }

    size_t getTasks() {
        return tasks;
    }

protected:

    void threadEntry (size_t i)
    {
//      	std::cout << "new db :" << &this->providerMap[i] << " _internal: " << &(this->providerMap[i]->getDB ()) << " with tid " << i << " has been generated\n";
		std::function <void (size_t,xew_directs *)> job;

        while (1)
        {
            start:
            {
                std::unique_lock <std::mutex> l (lock_);

                while (!shutdown_ && !_prod_ready && jobs_.empty()) {
                    condVar_.wait (l);
                }
				
                if (shutdown_)
                {
                    // we are shutting down
//                    std::cerr << "Thread " << i << " terminates" << std::endl;
                    return;
                }
                if(jobs_.empty() ){
                  // No jobs to do, wait until shutdown
                  goto start;
                }

                job = std::move (jobs_.front ());
                _cnt[i]++;
                jobs_.pop();

            }
            // Do the job without holding any locks
            job (i, this->providerMap[i]);
            done ++ ;
        }

    }
	
    std::unordered_map<size_t,size_t> _cnt;
    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    bool _prod_ready;
    std::queue <std::function <void (size_t,xew_directs *)>> jobs_;
    std::vector <std::thread> threads_;
    std::atomic<size_t> done{0};
    std::atomic<size_t> tasks{0};
};

class ThreadDBWrapper{
  //static xaw_directs db;
  int thread_id;
  bool is_init = false; 
 public:
  static xaw_directs replay_thread_wrapper_db;
  ThreadDBWrapper() = delete;
  ThreadDBWrapper(int thread_id){
    this->thread_id = thread_id;
  } 
  xaw_directs& getDB(){
    if(!is_init){
        std::cout << "init thread info per thread once, thread_id: " << this->thread_id << std::endl;
        TThread::set_id(this->thread_id);
        actual_directs::thread_init();
        is_init = true;
    }
    return replay_thread_wrapper_db;
  }
};

class TSharedThreadPool
{
    ;
public:

    TSharedThreadPool (int threads) : shutdown_ (false),produced_(false)
    {
        // Create the specified number of threads
//          threads_.reserve (threads);
	 //ThreadDBWrapper::initDb(); 
         for (int i = 0; i < threads; ++i) {
		 this->_mapping[i] = new ThreadDBWrapper(i);
	 }
    }
    
    void initPool(int threads, bool taskset=true) {
	  for (int i = 0; i < threads; ++i) {
		this->threads_.emplace_back (std::thread ([this, i] () {
		  this->threadEntry (i);
		}));
	  }
         
          if (taskset) {
              for (int i = 0; i < threads; ++i) {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(i, &cpuset);
                int rc = pthread_setaffinity_np(threads_[i].native_handle(),
                                                sizeof(cpu_set_t), &cpuset);
                if (rc != 0) {
                    std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
                }
              }
         }
	}

    ~TSharedThreadPool () { }

    /**
     * @brief after each callback from paxos, joinAll must called
     */
    void joinAll() {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);

            produced_ = true;
            condVar_.notify_all();
        }
    }
    
    /**
     * @brief must use this API to ensure threads gets closed
     * @param threads
     */
    void closeAll(int threads) {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);

            shutdown_ = true;
            condVar_.notify_all();
        }

        // Wait for all threads to stop
        // std::cerr << "JOIN threads" << std::endl;
        waitDone();
        for (size_t i=0;i<this->threads_.size();i++) {
		  this->threads_[i].join ();
		  delete(this->_mapping[i]);
		}
	//delete(this->_mapping[i]);
    }

    void doJob (std::function <void (int,ThreadDBWrapper*)> func)
    {
        tasks ++ ;
        // Place a job on the queue and unblock a thread
        std::unique_lock <std::mutex> l (lock_);

        jobs_.emplace (std::move (func));
        condVar_.notify_one();
    }

    size_t getNotRunningSize() {
        return jobs_.size() ;
    }

    void waitDone() {
        while (tasks != done) {};
        done = 0 ;
        tasks = 0 ;
    }

    int getDone() {
        return done ;
    }

    ThreadDBWrapper* getDBWrapper(int par_id) {
        return this->_mapping[par_id];
    }

    int getTasks() {
        return tasks;
    }
    
    void genStats(bool clear=false) {
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::cout << "------------------------------------------\n";
      
      int _count =0;
      for(auto &each : this->_stats_cnt){
          std::cout << each.first << " , " << each.second << std::endl;
          _count+=each.second;
	  }
	  std::cout << "------------------------------------------\n";
	  std::cout << "------------------------------------------\n";
	  if(clear)
	  	this->_stats_cnt = {};
      std::cout << " Total job done : " << _count << std::endl;
      std::cout << "------------------------------------------\n";
    }

protected:

    void threadEntry (int i)
    {
        std::function <void (int, ThreadDBWrapper*)> job;

        while (1)
        {
          	start:
            {
                std::unique_lock <std::mutex> l (lock_);

                while (!shutdown_ && !produced_ && jobs_.empty()) {
                    condVar_.wait (l);
                }
				
                if (shutdown_)
                {
                    // we are shutting down
//                    std::cerr << "Thread " << i << " terminates" << std::endl;
                    return;
                }
                
                if(jobs_.empty() ){
                  // No jobs to do, wait until shutdown
                  goto start;
                }

                job = std::move (jobs_.front ());
                this->_stats_cnt[i]++;
                jobs_.pop();
            }
            // Do the job without holding any locks
            job (i, this->_mapping[i]);
            done ++ ;
        }

    }
    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    bool produced_;
    
    std::unordered_map<int,int> _stats_cnt;
    std::unordered_map<int,ThreadDBWrapper*> _mapping;
    std::queue <std::function <void (int,ThreadDBWrapper *)>> jobs_;
    std::vector <std::thread> threads_;
    std::atomic<int> done{0};
    std::atomic<int> tasks{0};
};

class SharedThreadPool
{
public:

    SharedThreadPool (int threads) : shutdown_ (false)
    {
        // Create the specified number of threads
        threads_.reserve (threads);
        for (int i = 0; i < threads; ++i)
            threads_.emplace_back (std::bind (&SharedThreadPool::threadEntry, this, i));
    }

    ~SharedThreadPool () { }

    void joinAll() {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);

            shutdown_ = true;
            condVar_.notify_all();
        }

        // Wait for all threads to stop
        // std::cerr << "JOIN threads" << std::endl;
        for (auto& thread : threads_)
            thread.join();
    }

    void doJob (std::function <void (void)> func)
    {
        tasks ++ ;
        // Place a job on the queue and unblock a thread
        std::unique_lock <std::mutex> l (lock_);

        jobs_.emplace (std::move (func));
        condVar_.notify_one();
    }

    size_t getNotRunningSize() {
        return jobs_.size() ;
    }

    void waitDone() {
        while (tasks != done) {};
        done = 0 ;
        tasks = 0 ;
    }

    int getDone() {
        return done ;
    }

    int getTasks() {
        return tasks;
    }

protected:

    void threadEntry (int i)
    {
        std::function <void (void)> job;

        while (1)
        {
            {
                std::unique_lock <std::mutex> l (lock_);

                while (! shutdown_ && jobs_.empty()) {
                    condVar_.wait (l);
                }

                if (jobs_.empty ())
                {
                    // No jobs to do and we are shutting down
                    std::cerr << "Thread " << i << " terminates" << std::endl;
                    return;
                }

                job = std::move (jobs_.front ());
                jobs_.pop();

            }
            // Do the job without holding any locks
            job ();
            done ++ ;
        }

    }

    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    std::queue <std::function <void (void)>> jobs_;
    std::vector <std::thread> threads_;
    std::atomic<int> done{0};
    std::atomic<int> tasks{0};
};

class ThreadDBWrapperMbta {
    int thread_id;
    bool is_init = false;
public:
    static abstract_db* replay_thread_wrapper_db;
    ThreadDBWrapperMbta() = delete;
    ThreadDBWrapperMbta(int thread_id){
        this->thread_id = thread_id;
    }
    abstract_db * getDB(){
        if(!is_init){
            std::cout << "init thread info per thread once, thread_id: " << this->thread_id << std::endl;
            TThread::set_id(this->thread_id);
            actual_directs::thread_init();
            is_init = true;
        }
        return replay_thread_wrapper_db;
    }
};


class TSharedThreadPoolMbta
{
    ;
public:

    TSharedThreadPoolMbta (int threads) : shutdown_ (false),produced_(false)
    {
        // Create the specified number of threads
        for (int i = 0; i < threads; ++i) {
            this->_mapping[i] = new ThreadDBWrapperMbta(i);
        }
    }

    void initPool(int threads, bool taskset=true) {
        for (int i = 0; i < threads; ++i) {
            this->threads_.emplace_back (std::thread ([this, i] () {
                this->threadEntry (i);
            }));
        }

        if (taskset) {
            for (int i = 0; i < threads; ++i) {
                cpu_set_t cpuset;
                CPU_ZERO(&cpuset);
                CPU_SET(i, &cpuset);
                int rc = pthread_setaffinity_np(threads_[i].native_handle(),
                                                sizeof(cpu_set_t), &cpuset);
                if (rc != 0) {
                    std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
                }
            }
        }
    }

    ~TSharedThreadPoolMbta () { }

    /**
     * @brief after each callback from paxos, joinAll must called
     */
    void joinAll() {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);

            produced_ = true;
            condVar_.notify_all();
        }
    }

    /**
     * @brief must use this API to ensure threads gets closed
     * @param threads
     */
    void closeAll(int threads) {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);

            shutdown_ = true;
            condVar_.notify_all();
        }

        // Wait for all threads to stop
        waitDone();
        for (size_t i=0;i<this->threads_.size();i++) {
            this->threads_[i].join ();
            delete(this->_mapping[i]);
        }
    }

    void doJob (std::function <void (int,ThreadDBWrapperMbta*)> func)
    {
        tasks ++ ;
        // Place a job on the queue and unblock a thread
        std::unique_lock <std::mutex> l (lock_);

        jobs_.emplace (std::move (func));
        condVar_.notify_one();
    }

    size_t getNotRunningSize() {
        return jobs_.size() ;
    }

    void waitDone() {
        while (tasks != done) {};
        done = 0 ;
        tasks = 0 ;
    }

    int getDone() {
        return done ;
    }

    ThreadDBWrapperMbta* getDBWrapper(int par_id) {
        return this->_mapping[par_id];
    }

    int getTasks() {
        return tasks;
    }

    void genStats(bool clear=false) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        std::cout << "------------------------------------------\n";

        int _count =0;
        for(auto &each : this->_stats_cnt){
            std::cout << each.first << " , " << each.second << std::endl;
            _count+=each.second;
        }
        std::cout << "------------------------------------------\n";
        std::cout << "------------------------------------------\n";
        if(clear)
            this->_stats_cnt = {};
        std::cout << " Total job done : " << _count << std::endl;
        std::cout << "------------------------------------------\n";
    }

protected:

    void threadEntry (int i)
    {
        std::function <void (int, ThreadDBWrapperMbta*)> job;

        while (1)
        {
            start:
            {
                std::unique_lock <std::mutex> l (lock_);

                while (!shutdown_ && !produced_ && jobs_.empty()) {
                    condVar_.wait (l);
                }

                if (shutdown_)
                {
                    // we are shutting down
                    return;
                }

                if(jobs_.empty() ){
                    // No jobs to do, wait until shutdown
                    goto start;
                }

                job = std::move (jobs_.front ());
                this->_stats_cnt[i]++;
                jobs_.pop();
            }
            // Do the job without holding any locks
            job (i, this->_mapping[i]);
            done ++ ;
        }

    }
    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    bool produced_;

    std::unordered_map<int,int> _stats_cnt;
    std::unordered_map<int,ThreadDBWrapperMbta*> _mapping;
    std::queue <std::function <void (int,ThreadDBWrapperMbta *)>> jobs_;
    std::vector <std::thread> threads_;
    std::atomic<int> done{0};
    std::atomic<int> tasks{0};
};

#ifndef MASS_DIRECT
class MassTreeWrapper{
  int wrapperID=0;
  actual_ds h;
  bool once_init;
 
 public:
  
  MassTreeWrapper(actual_ds& ref_, int ID);
  
  bool do_insert_op(const container_hello* arg, std::atomic<int> &success_counts,
	std::atomic<int> &insert_stats_count, std::atomic<int> &delete_stats_count,std::atomic<int> &failed);

  void insert(const container_hello *container, int wrapper_id);
};

class IndependentThread{
  std::vector<std::thread> worker_threads;
  std::map<int,MassTreeWrapper*> masstree_objs;
  int thread_count;
  static std::mutex m_;
  
 public:
  explicit IndependentThread(int t_count);
  
  static void independentWork(void* data, int id);
  
  void threadCaller(int t_id);

  void start_all(const std::map<int,MassTreeWrapper*>& objs);
  
  void add_work(const container_hello* container,int mass_id);
  
  static void set_finish_status(){
    finish_all = true;
  }
  
  static void* wait_for_finish (void *);
  static bool finish_all;
};


int getRandomNumber(int TOTAL_MASS_TREE_COUNT);
void add_new_work(IndependentThread* tt,const container_hello* con,int mass_id);
#endif

