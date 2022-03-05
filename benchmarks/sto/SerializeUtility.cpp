//
// Created by mrityunjay kumar on 2019-08-01.
//

#include <iostream>
#include <map>
#include <thread>
#include <sstream>
#include <fstream>
#include <string>
#include <cstring>
#include <vector>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <vector>
#include <fcntl.h>
#include <functional>

#include "SerializeUtility.h"

std::map<int,LoggingSerializer *> LoggingSerializer::_instance{};
std::map<std::string, pthread_t> LoggingSerializer::tm_{};
std::map<int,LoggingSerializerDestroyer> LoggingSerializer::_destroyer{};
std::map<int,bool> LoggingSerializer::_run_status{};
//std::map<int,std::ofstream> LoggingSerializer::m_Logfile{};

uint64_t timeSinceEpochMillisec() {
  return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count();
//  return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

LoggingSerializerDestroyer::LoggingSerializerDestroyer(LoggingSerializer *s)
{
    _singleton = s;
}

LoggingSerializerDestroyer::~LoggingSerializerDestroyer()
{
    _singleton->stop(0);
    delete _singleton;
}

void LoggingSerializerDestroyer::SetLoggingSerializer(LoggingSerializer *s)
{
    _singleton = s;
}

//
///* write the range and force it out of the page cache */
//static void discard(int fd, size_t offset, size_t len) {
//    /* contrary to the former call this will block, force write
//     * out the old range, then tell the OS we don't need it
//     * anymore */
//    sync_file_range(fd, offset, len,
//            SYNC_FILE_RANGE_WAIT_BEFORE |
//            SYNC_FILE_RANGE_WRITE
//            | SYNC_FILE_RANGE_WAIT_AFTER
//            );
//    posix_fadvise(fd, offset, len, POSIX_FADV_DONTNEED);
//}


LoggingSerializer::LoggingSerializer (int tid) {
    thread_id = tid;
    MAX_MB_SUPPORTED = GLOBAL_MAX_MB_SUPPORTED;
    re_align(MAX_MB_SUPPORTED);
    std::string m_sFileName = std::string("/home/jay/logs/") + "Log-ThreadID:" + \
                                                                std::to_string (tid)+ ".txt";
    __fd = open(m_sFileName.c_str(),O_WRONLY|O_CREAT|O_TRUNC,0644);
    last_sec = timeSinceEpochMillisec();
    std::thread t1(std::bind(&LoggingSerializer::flushD,this));
    native_h = t1.native_handle();
	t1.detach ();
	_run_status[tid]= true;
}

void LoggingSerializer::re_align(size_t new_size, const std::vector<double>& arr){
    percents.clear();
//    buffer = new char[new_size];
    for (float i : arr) {
      size_t __MB = size_t (double(new_size)*i);
      percents.emplace_back(__MB);
    }
}

void LoggingSerializer::stop (int soft) {
  	fsync (__fd);
	_run_status[thread_id]= false;
//  	pthread_cancel(native_h);
}

LoggingSerializer *LoggingSerializer::Instance(int tid)
{
    LoggingSerializer *ins;
    auto search = _instance.find(tid);
    if (search == _instance.end ())
    {
       ins = new LoggingSerializer(tid);
       _destroyer[tid].SetLoggingSerializer(ins);
       _instance[tid] = ins;
    }else{
         ins = search->second;
    }
    return ins;
}

void LoggingSerializer::Log (const std::string &atr) {
    const char *p = atr.data ();
    size_t len = atr.size();
    Log (p,len);
}

bool LoggingSerializer::check_write(){
//    return (wpos > percents[p_index]);
	return true;
}

bool LoggingSerializer::mem_check(){
    return true;
}

void LoggingSerializer::TimedLog (const char* sMessage,size_t len) {
    Log (sMessage,len);
}

void LoggingSerializer::Log (const char *arr, size_t len) {
    size_t remaining=len;
    size_t w=0;

    for (int i = 0; i < len / 8192; ++i) {
        write (__fd,arr + w,8192);
        w += 8192;
        remaining -= 8192;
      }

      if (remaining > 0) {
        write (__fd,arr + w,remaining);
      }
	last_sec = timeSinceEpochMillisec();
}

void LoggingSerializer::flushD()
{
  	this_sec = timeSinceEpochMillisec();
  	if(_run_status[thread_id] && (this_sec-last_sec>0)) {
	  fsync (__fd);
	}
}