//
// Created by mrityunjay kumar on 2019-07-02.
//

#pragma once

#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdarg>
#include <string>





//#ifdef TEST_REPLAY_SPEED_FEAT
//  #ifndef LOG_FOLDER
//  		#define LOG_FOLDER "/home/jay/prev_logs/"
//  #endif
//#elif TEST_WRITE_SPEED_FEAT
//  #ifndef LOG_FOLDER
//  		#define LOG_FOLDER "/home/jay/prev_logs/"
//  #endif
//#else
//  #ifndef LOG_FOLDER
//  		#define LOG_FOLDER "/home/AzureUser/prev_logs/"
//  #endif
//#endif

#ifndef LOG_FOLDER
	#define LOG_FOLDER "/home/AzureUser/prev_logs/"
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

struct LOGGING_CONST {
   static std::string WHOLE_LOG_STRING;
   static std::string WHOLE_INFO_STRING;
   
   static void setlog(int threads, int runTime){
	std::string SUFFIX_STRING = "GenLogThd"+std::to_string(threads)+".Time."+std::to_string(runTime)+"/";
	WHOLE_LOG_STRING = LOG_FOLDER + SUFFIX_STRING;
	WHOLE_INFO_STRING = LOG_FOLDER + SUFFIX_STRING + "info/";
   }
};

bool util_mkpath( const std::string& path );

/**
*   Singleton Logger Class.
*/
class OutputDataSerializer
{
public:
    uint64_t UUID();
    /**
    *   Logs a message
    *   @param _oss message to be logged.
    */
//    void Log(std::ostream& _oss);
    /**
    *   Logs a message
    *   @param sMessage message to be logged.
    */
    void TimedLog (const char* sMessage,size_t len,size_t count);
    void Log(const char* sMessage,size_t len);
    void LLog(const char* sMessage,size_t len);
    void Log(const std::string& sMessage);
    void Log(const std::vector<long long unsigned int>* str);
    void BloopLog();
    void MapWriter(const std::map<std::string,long>& fileMap);
    void MapWriter(const std::unordered_map<std::string,long>& fileMap);
    /**
    *   Variable Length Logger function
    *   @param format string for the message to be logged.
    */
//    void Log(const char * format);
    /**
    *   << overloaded function to Logs a message
    *   @param sMessage message to be logged.
    */
    OutputDataSerializer& operator<<(const std::string& sMessage);
    /**
    *   Funtion to create the instance of logger class.
    *   @return singleton object of Clogger class..
    */
    static OutputDataSerializer* GetLogger(int thread_id,const std::string& parent_folder_name);
    
 public:
  	static void flush_all();
  	static void close_all();
	OutputDataSerializer(int thread_id, const std::string& parent_folder_name,bool bloop=false);
	
    ~OutputDataSerializer(){
    	if(awesome_ostream.is_open()){
//    	  gmtx.lock ();
    	  std::cout << "Flushing for thread_id:" << thread_id << " Skip Close";
    	  awesome_ostream << std::flush;
//    	  awesome_ostream.close ();
//    	  gmtx.unlock ();
    	}
    }
    void flush(bool bloop= false);

//    static uint64_t _UUID;

private:

    size_t wpos{0};
    

    bool is_bloop_act;

    int thread_id;

//    size_t nBufSize;
//    char *buffer;

    static size_t BUFSIZE;

    static constexpr uint64_t increment_value = uint64_t(0x1000);
    /**
    *    Default constructor for the Logger class.
    */
    OutputDataSerializer(int thread_id);
    /**
    *   copy constructor for the Logger class.
    */
    OutputDataSerializer(const OutputDataSerializer&){};             // copy constructor is private
    /**
    *   assignment operator for the Logger class.
    */
    OutputDataSerializer& operator=(const OutputDataSerializer&){ return *this; };  // assignment operator is private

    /**
    *   Singleton logger class object pointer.
    **/
    static std::unordered_map<int, OutputDataSerializer*> m_pThis;
    //static OutputDataSerializer* m_pThis;
    /**
    *   Log file stream object.
    **/
    std::ofstream awesome_ostream;
    static std::unordered_map<int,std::ofstream> m_FileStream;
    static std::unordered_map<int,std::stringstream> m_Logfile;
    static std::unordered_map<int,std::ofstream> m_FileStreamTimedLog;
    static std::unordered_map<int,std::FILE *> m_FileStream3;



};
