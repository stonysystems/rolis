
//
// Created by mrityunjay kumar on 2019-07-13.
//

#pragma once

#include <utility>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <map>
#include <unordered_map>
#include <unordered_set>

#include "Hashtable.hh"

#define MASS_DIRECT 1

#include "MassTrans.hh"

#include "simple_str.hh"
#include "randgen.hh"
//#include "MassDirect.hh"

#define DS 1
#define USE_STRINGS 1

//#define TEST_REPLAY_SPEED_FEAT

#if DS == 0
#if USE_STRINGS == 1
typedef Hashtable<std::string, std::string, false, 1000000, simple_str> ds;
#else
typedef Hashtable<int, std::string, false, 1000000, simple_str> ds;
#endif
#else

//#ifndef MASS_DIRECT
//typedef MassTrans<std::string, versioned_str_struct, false/*opacity*/> actual_ds;
//#else
//typedef MassTrans<std::string, versioned_str_struct, false/*opacity*/> actual_directs;
//#endif
//
//#ifndef MASS_DIRECT
//typedef std::map<long, actual_ds*> ds;
//typedef std::map<long, actual_ds> new_ds;
//#else
//typedef std::unordered_map<long, actual_directs> new_directs;
//#endif

typedef MassTrans<std::string, versioned_str_struct, false/*opacity*/> actual_directs;
typedef std::unordered_map<long, actual_directs> new_directs;

#endif

//static std::atomic<uint64_t> MB_IN_ACTION;

static const std::size_t kB = 1024;
static const std::size_t MB = 1024 * kB;
static const std::size_t GB = 1024 * MB;

static std::string SplitFilename (const std::string& str) {
  unsigned found = str.find_last_of("//");
  return str.substr(found+1);
}

class OneLog{
  typedef unsigned long long int ULL_I;
  size_t ULL_LEN = sizeof (ULL_I);
  size_t UC_LEN = sizeof (unsigned char);
  size_t USI_LEN = sizeof (unsigned short int);
  
  public:
  ULL_I k =0;
  ULL_I v = 0;
  unsigned short int table_id=0;
  ULL_I insert_true=0;
  unsigned char delete_true=0;
  ULL_I cid=0;
  
  std::string str_value="";
  std::string str_key = "";
  
 public:
  OneLog(char *buffer,size_t& rw, const size_t& _cid, const std::unordered_set<long>& table_set){
	k =0;
	v = 0;
	table_id=0;
	insert_true=0;
	delete_true=0;
    cid=_cid;
    
    memcpy ((char *) &k, buffer + rw, ULL_LEN);
	rw += ULL_LEN; //8
	
	memcpy ((char *) &v, buffer + rw, ULL_LEN);
	rw += ULL_LEN; // 8
	
	memcpy ((char *) &table_id, buffer + rw, USI_LEN);
	// verify
	if(table_set.find (table_id) == table_set.end ()){
	  std::cout << "Massive Error!!! " << " Table id= " << table_id << " doesn't exist" << std::endl;
	  exit(1);
	}
	rw += USI_LEN; // 2
	
	memcpy ((char *) &delete_true, buffer + rw, UC_LEN);
	rw += UC_LEN;  // 1
  }
};

class OneLogGroup {
	std::vector<OneLog*> _list;
 public:
    explicit OneLogGroup(std::vector<OneLog*>& actualList){
      this->_list = std::ref(actualList);
    }
    
	void add_one_container(char *buffer,size_t &currentReadPointer,
		                   const size_t& lenToCover,
		                   const size_t& cid, const size_t& stopPointer,
		                   const std::unordered_set<long>& table_set){
	  while(currentReadPointer < stopPointer) {
		auto oneLog = new OneLog (buffer, currentReadPointer, cid, table_set);
		_list.emplace_back (oneLog);
	  }
	}
	
	std::vector<OneLog*>& GetItems(){
	  return _list;
  	}
};

class one_container{
  typedef unsigned long long int ULL_I;
  size_t ULL_LEN = sizeof (ULL_I);
  size_t UC_LEN = sizeof (unsigned char);
  size_t USI_LEN = sizeof (unsigned short int);
  
  public:
//  static size_t dread;
  ULL_I k =0;
  ULL_I v = 0;
  unsigned short int table_id=0;
  ULL_I insert_true=0;
  unsigned char delete_true=0;
  ULL_I cid=0;
  std::string str_value="";
  std::string str_key = "";
  bool exists=false;
  
  std::string &getValue(){
    return str_value;
  }
  
  one_container(char *buffer,size_t& rw){
    k =0;
    v = 0;
    table_id=0;
    insert_true=0;
    delete_true=0;
    cid=0;
    
    memcpy ((char *) &cid, buffer + rw, ULL_LEN);
	rw += ULL_LEN;
	
	memcpy ((char *) &k, buffer + rw, ULL_LEN);
	rw += ULL_LEN;
	
	memcpy ((char *) &v, buffer + rw, ULL_LEN);
	rw += ULL_LEN;
	
	memcpy ((char *) &table_id, buffer + rw, USI_LEN);
	rw += USI_LEN;
	
	memcpy ((char *) &delete_true, buffer + rw, UC_LEN);
	rw += UC_LEN;
  }
};

// sizeof = 8
#define DECL_OFFSET_K 0
// sizeof = 8
#define DECL_OFFSET_V 8
// sizeof = 2
#define DECL_OFFSET_TABLE_ID 16
#define DECL_OPTIMAL_SIZE 18

struct WrappedLog{
  char* arr;
  unsigned long long int cid;
  unsigned long long int len;
};

// support flexible K-V, each WrappedLogV2 contains one transaction with operations
struct WrappedLogV2{
    char* arr;
    unsigned long long int cid;
    unsigned long long int count;   // the count of K-V operations
    unsigned long long int len;   // the len of K-V operations
};

class container_hello{
  	size_t start;
  	size_t end;
  	const std::string file_name;
  	FILE *fin;
  	int r_thrd_count;
  	long table_id;
  	std::vector<one_container*> _list;
  	
  	
 public:
  	
  	container_hello(size_t s,size_t e,std::string name,FILE *fp,int trh_cnt, long tid):
  	start(s),end(e),file_name(std::move(name)),fin(fp),r_thrd_count(trh_cnt),table_id(tid){
  	  _list.clear ();
  	  _list = {};
  	};
  	
  	void add_one_container(one_container* one){
  	  _list.emplace_back (one);
  	}
  	
  	void bulk_add_containers(std::vector<one_container*>& passed_list){
  	  _list.insert( _list.end(), passed_list.begin(), passed_list.end() );
  	}
  	
  	long GetTableID()const {
	  return table_id;
  	}
  	
  	long GetThreadID()const {
	  return r_thrd_count;
  	}
  	
  	std::vector<one_container*> GetItems() const{
	  return _list;
  	}
  	
  	long GetContainerCounts() const {
	  return _list.size ();
  	}
  	
  	void free_all(){
  	  for(auto &i : _list){
  	    delete i;
  	  }
  	  _list.clear ();
  	}
  	
  	std::string GetFilename() const{
	  return file_name;
  	}
  	
  	std::string GetPreetyFName() const {
  	  return SplitFilename(file_name);
  	}
  	
  	size_t GetFileEndPos() const{
  	  return end;
  	}
  	
  	size_t GetFileStartPos() const{
  	  return start;
  	}
  	
  	const std::string size_in_MB() const{
	  return std::to_string(double(end-start)/double(MB))+" MB";
  	}
  	
  	int run_thread_count() const{
	  return r_thrd_count;
  	}
};

//#ifndef MASS_DIRECT
//class Function_pool
//{
//
//private:
//
//    std::queue<std::pair<std::function<void(ds&,const container_hello*)>,std::pair<ds,const container_hello*>>> m_function_queue;
//    std::mutex m_lock;
//    std::condition_variable m_data_condition;
//    std::atomic<bool> m_accept_functions;
//
//public:
//
//    Function_pool();
//    ~Function_pool();
//    void push(const std::function<void(ds&,const container_hello*)>& func, ds &h,const container_hello* filename);
////    void push(const std::function<void(const container_hello*)>& func,const container_hello* filename);
//    void done();
//    void infinite_loop_func();
//};
//#endif

