//
// Created by mrityunjay kumar on 2019-07-02.
//
#include <algorithm>
#include <functional>
#include <memory>
#include <type_traits>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <string>
#include <ctime>
#include <mutex>
#include <queue>
#include <map>
#include <unordered_map>
#include <wchar.h>
#include <thread>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/fcntl.h>
#include <fstream>
#include <unistd.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

// C++ STD
#include <iostream>
#include <cstring>
#include <stdint.h>
#include <cassert>

#include "OutputDataSerializer.h"

std::mutex logGetterMtx;

const std::size_t kB = 1024;
const std::size_t MB = 1024 * kB;
const std::size_t GB = 1024 * MB;
const std::size_t WINDOW = 8 * MB;


namespace Util
{
    // Get current date/time, format is YYYY-MM-DD.HH:mm:ss
    const std::string CurrentDateTime()
    {
        char       buf[80];
        std::time_t t = std::time(0);   // get time now
        std::tm* now = std::localtime(&t);
        strftime(buf, sizeof(buf), "%Y_%m_%d_%X", now);
        return std::string(buf);
    }
}

std::string LOGGING_CONST::WHOLE_LOG_STRING="";
std::string LOGGING_CONST::WHOLE_INFO_STRING="";
//const std::string OutputDataSerializer::m_sFileName = std::string("/home/jay/logs/") + Util::CurrentDateTime() + "_LOG.txt";
//uint64_t __attribute__((aligned(128))) OutputDataSerializer::_UUID = 2 * OutputDataSerializer::increment_value;
//pthread_mutex_t OutputDataSerializer::m_oMutex= PTHREAD_MUTEX_INITIALIZER;
std::unordered_map<int, OutputDataSerializer*> OutputDataSerializer::m_pThis={};
std::unordered_map<int, std::stringstream> OutputDataSerializer::m_Logfile={};
std::unordered_map<int, std::ofstream> OutputDataSerializer::m_FileStream={};
std::unordered_map<int, std::ofstream > OutputDataSerializer::m_FileStreamTimedLog={};
std::unordered_map<int,std::FILE *> OutputDataSerializer::m_FileStream3={};
size_t OutputDataSerializer::BUFSIZE=2048;

bool util_mkpath( const std::string& path )
{
  	if(path == ""){
  	  return false;
  	}
    bool bSuccess = false;
    int nRC = ::mkdir( path.c_str(), 0775 );
    if( nRC == -1 )
    {
        switch( errno )
        {
            case ENOENT:
                //parent didn't exist, try to create it
                if( util_mkpath( path.substr(0, path.find_last_of('/')) ) )
                    //Now, try to create again.
                    bSuccess = 0 == ::mkdir( path.c_str(), 0775 );
                else
                    bSuccess = false;
                break;
            case EEXIST:
                //Done!
                bSuccess = true;
                break;
            default:
                bSuccess = false;
                break;
        }
    }
    else
        bSuccess = true;
    return bSuccess;
}

OutputDataSerializer::OutputDataSerializer(int inp_thread_id)
{
  	this->is_bloop_act = false;
    this->thread_id = inp_thread_id;
    this->wpos = 0;
}


OutputDataSerializer::OutputDataSerializer(int inp_thread_id,const std::string& parent_folder_name, bool bloop)
{
    this->thread_id = inp_thread_id;
    this->wpos = 0;
    this->is_bloop_act = bloop;
    
    util_mkpath(parent_folder_name);
	std::string _m_sFileName = parent_folder_name + "Log-ThreadID:" + \
                                                        std::to_string (inp_thread_id) + ".txt";
	if(bloop)
		awesome_ostream.open (_m_sFileName.c_str (), std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
}


OutputDataSerializer* OutputDataSerializer::GetLogger(int inp_thread_id,const std::string& parent_folder_name){
//    int thread_id = 1;
    OutputDataSerializer *instance=nullptr;
    logGetterMtx.lock ();
    std::string m_sFileName;
    auto search = m_pThis.find(inp_thread_id);
    if (search == m_pThis.end()){
        instance = new OutputDataSerializer(inp_thread_id);
        m_pThis.insert (std::pair<int, OutputDataSerializer*>(inp_thread_id, instance));
        util_mkpath(parent_folder_name);
        std::string _m_sFileName = parent_folder_name + "Log-ThreadID-" + std::to_string (inp_thread_id)+ ".txt";
//        std::ios_base::sync_with_stdio(false);
        m_FileStream[inp_thread_id].open(_m_sFileName.c_str(), std::ofstream::out | std::ofstream::trunc|std::ofstream::binary);
        
	  	if(parent_folder_name == std::string(INFO_LOG_FOLDER_NAME)){
	  	  logGetterMtx.unlock ();
	  	  return instance;
	  	}
	  
        #ifdef TEST_WRITE_SPEED_FEAT
        
        std::string timed_parent_folder_name = parent_folder_name + "/timedlog/";
        util_mkpath (timed_parent_folder_name);
        #endif
        
        
        #ifdef TEST_WRITE_SPEED_FEAT
        
        std::string _m_sFileNameNamed = timed_parent_folder_name + "Log-ThreadID:" + \
                                                                    std::to_string (inp_thread_id)+ ".csv";
        
        m_FileStreamTimedLog[inp_thread_id].open(_m_sFileNameNamed.c_str(), std::ofstream::out | std::ofstream::trunc|std::ofstream::binary);
		#endif
//        std::thread t1(std::bind(&OutputDataSerializer::flush,instance));
//        t1.detach ();

//        m_FileStream[inp_thread_id].rdbuf ()->pubsetbuf (instance->buffer,instance->nBufSize);

    } else {
        instance = search->second;
    }
    logGetterMtx.unlock ();
    return instance;
}


void OutputDataSerializer::Log(const std::string &str)
{
  	if(this->is_bloop_act)
  	  awesome_ostream << str << ":";
  	else
  	  m_FileStream[this->thread_id] << str;
}

void OutputDataSerializer::Log(const std::vector<long long unsigned int>* str)
{
//    m_Logfile[this->thread_id] << str->data () << '\n';
}

void OutputDataSerializer::BloopLog () {
  	auto time_stamp = std::to_string(static_cast<long int> (std::time(0)));
    Log(time_stamp);
}

void OutputDataSerializer::TimedLog (const char* sMessage,size_t len,size_t count) {
  	#ifdef TEST_WRITE_SPEED_FEAT
    auto startTime = std::chrono::high_resolution_clock::now();
	#endif
    Log (sMessage,len);
    #ifdef TEST_WRITE_SPEED_FEAT
    auto endTime = std::chrono::high_resolution_clock::now();
    auto time_spent =std::chrono::duration_cast<std::chrono::nanoseconds>(endTime - startTime).count();
    if(time_spent == 0.0)     {
	  return;      
    }
    long double yyy = (float (len) / float(MB) / float(time_spent)) * 1000.0 * 1000.0 * 1000.0 ;
    std::string string_to_write = std::string("XXX,")+std::to_string(yyy)+std::string(",") + std::to_string(time_spent)+","+
          std::to_string(static_cast<long int> (std::time(0)))+","+std::to_string(len)+","+std::to_string(count)+"\n";
	
	m_FileStreamTimedLog[this->thread_id].write(string_to_write.c_str(),string_to_write.length());
	#endif
}

void OutputDataSerializer::MapWriter(const std::map<std::string,long>& fileMap){
    for(auto& iter:fileMap)
    {
        m_FileStream[this->thread_id]<<iter.first<<","<<iter.second;
        m_FileStream[this->thread_id]<<"\n";
    }
//    m_FileStream[this->thread_id].flush ();
}

void OutputDataSerializer::MapWriter(const std::unordered_map<std::string,long>& fileMap)
{
    for(auto& iter:fileMap)
    {
        m_FileStream[this->thread_id]<<iter.first<<","<<iter.second;
        m_FileStream[this->thread_id]<<"\n";
    }
//    m_FileStream[this->thread_id].flush ();
}

void OutputDataSerializer::LLog (const char* sMessage,size_t len) {  // dumps tnx into a file stream
//  	gmtx.lock ();
  	if(awesome_ostream.is_open())
    	awesome_ostream.write (sMessage,len);
  	else{
  	  std::cout << "Writing to a closed one, EXIT EXIT EXIT\n";
  	  exit(1);
  	}
//    gmtx.unlock ();
}

void OutputDataSerializer::Log (const char* sMessage,size_t len) {  // dumps tnx into a file stream
    m_FileStream[this->thread_id].write (sMessage,len);
}

void OutputDataSerializer::flush(bool bloop) {
  if (this->is_bloop_act){
	awesome_ostream.flush ();
  }
  else if(bloop) {
	m_FileStream[this->thread_id].flush ();
  }
}

void OutputDataSerializer::flush_all ()
{
//   std::cout << "flushed all started\n";
//   for(auto &each:m_FileStream )  {
//      each.second.flush ();
//   }
//   std::cout << "flushed all ok\n";
}

void OutputDataSerializer::close_all ()
{
  std::cout << "close handle all started\n";
   for(auto &each:m_FileStream )  {
//      sync_filebuf x(each.second);
//      each.second <<  std::endl;
	  each.second << std::flush;
      each.second.close ();
   }

  std::cout << "close_all all ok\n";
}
