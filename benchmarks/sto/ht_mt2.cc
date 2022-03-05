#ifndef HT_MT_2
#define HT_MT_2
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
#include "gperftools/profiler.h"
#include <stdlib.h>

#include "ReplayDB.h"
#include "../mbta_wrapper.hh"
#include <iostream>

using namespace std;

bool is_file_exist_mt2(std::string fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

// init db
abstract_db *ThreadDBWrapperMbta::replay_thread_wrapper_db = new mbta_wrapper;

void treplay_in_same_thread_scanned_logs() {

    TSharedThreadPoolMbta tpool_mbta (1);
    tpool_mbta.initPool (1);
    abstract_db * db = tpool_mbta.getDBWrapper(0)->getDB ();

    // 1. prepare scanned logs
    int pos=0;
    string msg="";
    for (int i=0; i<5; ++i) {
        string table_id = std::to_string(10001);
        string key = "key_" + std::to_string(i);
        string value = "value_" + std::to_string(i);

        msg += table_id + "|" \
                + std::to_string(key.size()) + "|" \
                + std::to_string(value.size()) + "|" \
                + key + value;
    }

    // 2. invoke scanned logs replaying
    std::cout << "replay cnt: " << treplay_in_same_thread_scanned_logs(0, (char*)msg.c_str(), msg.size(), db) ;

    // loop values in the file
    std::string path = "/home/weihshen/silo-sto/logs/";
    for(int fid=0;fid<4;fid++) {
        std::string file_name = std::string(path) + \
                                std::string("Log-ThreadID-") + \
                                std::to_string(fid) + std::string(".txt");

        if (!is_file_exist_mt2(file_name)) {
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

        int size = treplay_in_same_thread_scanned_logs(0, (char*)buffer, ret, db) ;
        std::cout << "put # of K-V: " << size << std::endl;
        close(fd) ;
    }


    tpool_mbta.closeAll(1);
}

int main(int argc, char **argv) {

    long file_count=1;
    size_t nthreads=1;
    std::string file_path="";
    std::string test_unit="";
    std::string optimized="";
    size_t buf_size = 48*1024*1024;
    std::string control="";
    nthreads = std::thread::hardware_concurrency();

    while (1) {
        static struct option long_options[] =
                {
                        {"file-count"                         , required_argument , 0                                     , 'f'} ,
                        {"file-path"                         , required_argument , 0                                     , 'p'} ,
                        {"test-uit"                         , required_argument , 0                                     , 'u'} ,
                        {"num-threads"                         , required_argument , 0                                     , 't'} ,
                        {"optimized"                         , required_argument , 0                                     , 'o'} ,
                        {"log-size"                         , required_argument , 0                                     , 'l'} ,
                        {"control"                         , required_argument , 0                                     , 'c'} ,
                        {0, 0, 0, 0}
                };

        int option_index = 0;
        int c = getopt_long(argc, argv, "r:", long_options, &option_index);

        if (c == -1)
            break;

        switch (c) {
            case 'f':
                file_count = strtoul (optarg, NULL, 10);
                break;
            case 'p':
                file_path = optarg;
                break;
            case 'u':
                test_unit = optarg;
                break;
            case 't':
                nthreads = strtoul (optarg, NULL, 10);
                break;
            case 'l':
                buf_size = strtoul (optarg, NULL, 10) ;
                break;
            case 'o':
                optimized = optarg;
                break;
            case 'c':
                control = optarg;
                break;

            default:
                file_count = 1;
                file_path = "";
                nthreads = 2;
                break;
        }
    }

    if (!test_unit.empty()) {
        if (test_unit.compare("treplay_in_same_thread_scanned_logs")==0) {
            treplay_in_same_thread_scanned_logs() ;
        } else {

        }

        return 0;
    }

    size_t max_buf_size=buf_size;
    if (control == "profiler")
        ProfilerStart("ht_mt2.prof");

    if (optimized == "1") {
        printf("replay mbta\n");
        abstract_db* db = NULL;
        db = new mbta_wrapper;
        start_replay_for_all_files_direct_mbta(file_count, file_path, nthreads, db);
    } else {
        printf("replay non-optimized\n");
        start_replay_for_all_files_direct_even(file_count, file_path, nthreads, max_buf_size);
    }

    if (control == "profiler")
      ProfilerStop();
}

#endif
