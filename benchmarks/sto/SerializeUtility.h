//
// Created by mrityunjay kumar on 2019-08-01.
//

#pragma once
#include <iostream>
#include <map>
#include <thread>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

const std::size_t kB = 1024;
const std::size_t MB = 1024 * kB;
const std::size_t GB = 1024 * MB;
const std::size_t MIN_DIFF_SUPPORTED = 100*MB;
const std::size_t GLOBAL_MAX_MB_SUPPORTED = 500*MB;
const std::size_t _10P_MB_SUPPORTED = 50*MB;
const std::size_t _20P_MB_SUPPORTED = 100*MB;
const std::size_t _30P_MB_SUPPORTED = 150*MB;
const std::size_t _40P_MB_SUPPORTED = 200*MB;


class LoggingSerializerDestroyer;

class LoggingSerializer
{
 public:
    int TID(){
        return thread_id;
    }
    void Log(const std::string& atr);
    void TimedLog (const char* sMessage,size_t len);
    void Log(const char *arr, size_t len);
    bool mem_check();
    bool check_write();
   static LoggingSerializer *Instance(int);
 protected:
   LoggingSerializer(int tid);
   void stop(int soft=1);
   friend class LoggingSerializerDestroyer;
   virtual ~LoggingSerializer(){}
 private:
    std::size_t MAX_MB_SUPPORTED=GLOBAL_MAX_MB_SUPPORTED;
    void re_align(size_t new_size, const std::vector<double>& arr={0.1,0.2,0.3,0.4});
    int thread_id;
//    char *buffer;
//   std::ostringstream _ss_stream;
//   std::ofstream m_Logfile;
   int __fd;
//   size_t flush_offset=0;
//   size_t global_offset=0;
   size_t wpos= 0;
   void flushD();
   size_t mem_cpy_count = 0;
   std::vector<size_t > percents;
   int p_index{0};
   uint64_t last_sec{};
   uint64_t this_sec{};
   size_t end = MAX_MB_SUPPORTED;
   pthread_t native_h;
   static std::map<std::string, pthread_t> tm_;
   static std::map<int,LoggingSerializer *> _instance;

   static std::map<int,LoggingSerializerDestroyer> _destroyer;
   static std::map<int,bool> _run_status;
//   static std::map<int,std::ofstream> m_Logfile;
};

class LoggingSerializerDestroyer
{
 public:
   LoggingSerializerDestroyer(LoggingSerializer *s = 0);
   ~LoggingSerializerDestroyer();
   void SetLoggingSerializer(LoggingSerializer *s);

 private:
   LoggingSerializer *_singleton;
};
