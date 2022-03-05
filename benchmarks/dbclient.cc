//
// Created by mrityunjay kumar on 12/31/20.
//
#include <functional>
#include <stdio.h>
#include <getopt.h>
#include <thread>
#include <iostream>
#include <unordered_set>
#include <random>
#include <string>
#include <unordered_map>
#include <chrono>
#include <map>
#include <queue>
#include <mutex>
#include <atomic>
#include <condition_variable>
#include <cstdlib>
#include <ctime>

#include "sto/function_pool.h"
#include "sto/ThreadPool.h"

static size_t LEN_ULL = sizeof (unsigned long long int );

static void keystore_encode3(std::string& s, unsigned long long int x, unsigned long long int y){
	memcpy ((void *)s.data(), &x, LEN_ULL);
	memcpy ((void *)(s.data()+LEN_ULL), &y, LEN_ULL);
}


static inline unsigned long long int keystore_decode3(const std::string& s, bool first= false){
	unsigned long long int x=0;
	if(first) {
	  memcpy (&x, (void *) (s.data ()), LEN_ULL);
	  return x;
	}else {
	  memcpy (&x, (void *) (s.data () + LEN_ULL), sizeof (unsigned long long int));
	  return x;
	}
}

static bool cmpFunc2(const std::string& newValue,const std::string& oldValue)
{
  unsigned long long int commit_id_new = keystore_decode3(newValue, true);
  unsigned long long int commit_id_old = keystore_decode3(oldValue, true);
  
  return commit_id_new > commit_id_old;
}

void replay_in_same_thread_object(xew_directs* db,const size_t& iter,const size_t& t){
  auto ts = static_cast<long int> (std::time(0));
  size_t table_id = 10001;
  db->insert(table_id, iter, t, "Iter" + std::to_string (iter)+"("+std::to_string(ts)+")", cmpFunc2);
}

void demo_func(SharedPoolTable &p,size_t start, size_t end){
  size_t iter=start;
  while(iter < end)
  {
	p.doJob ([=] (size_t publisher_thread_id, xew_directs *db) {
	  return replay_in_same_thread_object (db, iter, publisher_thread_id);
	});
	iter+=1;
  }
  p.waitDone ();
}

int main(int argc, char **argv) {
  unsigned int n = std::thread::hardware_concurrency ();
  std::cout << n << " concurrent threads are supported.\n";
  int replay_thr_count = 3;
  int iterations = 100;
  while (1) {
        static struct option long_options[] =
        {
          {"file-count"                         , required_argument , 0                                     , 'f'} ,
          {"num-threads"                         , required_argument , 0                                     , 't'} ,
          {0, 0, 0, 0}
        };

        int option_index = 0;
        int c = getopt_long(argc, argv, "f:t:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 'f':
                iterations = strtoul (optarg, NULL, 10);
                break;
            case 't':
                replay_thr_count = strtoul (optarg, NULL, 10);
                break;
            default:
                iterations = 100;
                replay_thr_count = 3;
                break;
        }
  }
  
  
  SharedPoolTable sharedThreads (replay_thr_count);
  sharedThreads.startAll ();
  for (size_t k = 0; k < iterations; k++)
	demo_func (sharedThreads, 4 * k, 4 * k + 3);
  
  for (size_t k = 0; k < iterations; k++)
	demo_func (sharedThreads, 4 * k, 4 * k + 3);
  
  sharedThreads.genStats (true);
  sharedThreads.closeAll ();
  return 0;
}