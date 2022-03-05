//
// Created by mrityunjay kumar on 2019-10-25.
//

#ifndef SILO_ONS_BENCHMARKS_STO_REPLAYDB_H_
#define SILO_ONS_BENCHMARKS_STO_REPLAYDB_H_

#include <iostream>
#include <assert.h>
#include <stdio.h>
#include <thread>
#include <iostream>
#include <queue>
#include <sstream>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>
#include <exception>
#include <fstream>
#include <thread>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <condition_variable>
#include <glob.h>
#include <sys/time.h>

#include "../abstract_db.h"
#include "function_pool.h"
#include "ThreadPool.h"

#ifndef MASS_DIRECT
void replay_process(char *log, size_t len, ds &database,int nthreads,std::map<std::string, long>& table_map);
//void start_replay_processing(int replay_thr_count);
//void start_replay_processing(int file_count,int replay_thr_count);
void start_replay_processing(int file_count,int replay_thr_count,ds& database);

void replay_in_same_thread(char *log, size_t len, new_ds &database,std::map<std::string, long>& table_map,int tid);
void start_replay_for_all_files(int file_count,int replay_thr_count,size_t max_buffer_allowed);
void replay_container(char *log, size_t len, int nthreads,std::map<std::string, long>& table_map, IndependentThread* processor);
#endif
void replay_in_shared_threads_opt(
	SharedThreadPool &p,
	size_t workers,
	const char *buffer,
	size_t len,
	const std::unordered_set<long>& all_files_names_temp,
	new_directs& database);
double treplay_in_shared_threads(TSharedThreadPool &tp, size_t workers, char *log, size_t total_bytes);
size_t Treplay_in_same_thread_wrapper_pointer_even(int thread_id, size_t offset, char *base, size_t entries_cnt_per_core, new_directs& database) ;
size_t treplay_in_same_thread_opt(size_t par_id, char *buffer, size_t len, new_directs& database, const std::unordered_set<long>& all_files_names_temp);
size_t treplay_in_same_thread_opt_mbta(size_t par_id, char *buffer, size_t len, abstract_db* db, const std::unordered_set<long>& all_files_names_temp);
unsigned long long int treplay_in_same_thread_opt_cid(char *buffer, size_t len) ;
void start_replay_for_all_files_direct_wo_threadpool(int file_count, std::string file_path, int replay_thr_count);
void start_replay_for_all_files_direct_mbta(int file_count, std::string file_path, int replay_thr_count, abstract_db *db);
void start_replay_for_all_files_direct_wo(int file_count, std::string file_path, int replay_thr_count);
double replay_in_multi_threads(char *log, size_t total_bytes, int replay_thr_count, new_directs& database,std::map<std::string, long>& table_map);
double replay_in_shared_threads(SharedThreadPool &p, size_t workers, char *log, size_t total_bytes, new_directs& database) ;
// deprecated API, database is a value parameter
size_t replay_in_same_thread_wrapper_pointer_even(int thread_id, size_t offset, char *base, size_t entries_cnt_per_core, new_directs database);
int replay_in_same_thread_wrapper(int thread_id, int offset,
	                              const std::vector<std::pair<char *,int>>& buffers,
                                  bool end_true, int stride,
                                  new_directs database,
                                  std::map<std::string, long> table_map);
void replay_in_same_thread_direct(char *log, size_t len, new_directs& database,std::map<std::string, long>& table_map, int tid);
void start_replay_for_all_files_direct(int file_count, std::string file_path, int replay_thr_count,size_t max_buffer_allowed);
void start_replay_for_all_files_direct_even(int file_count, std::string file_path, int replay_thr_count,size_t max_buffer_allowed);
uint64_t timeSinceEpochMillisecX() ;

// get the latest commit_id from logs - 05/05/2021 weihshen
unsigned long long int get_latest_commit_id(char *buffer, size_t len) ;
// replay logs with flexible K and V - 05/05/2021 weihshen
size_t treplay_in_same_thread_opt_mbta_v2(size_t par_id, char *buffer, size_t len, abstract_db* db);
// replay scanned logs - 11/19/2021 weihshen
size_t treplay_in_same_thread_scanned_logs(size_t par_id, char *buffer, size_t len, abstract_db* db) ;

#endif //SILO_ONS_BENCHMARKS_STO_REPLAYDB_H_
