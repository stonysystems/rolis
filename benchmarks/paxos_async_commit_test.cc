#include <iostream>
#include <cstring>
#include <vector>
#include <getopt.h>
#include <chrono>
#include <thread>
#include "deptran/s_main.h"

using namespace std;

void serialize_util_wrapper(size_t logs_to_commit, int thread_id, size_t par_id) {
    for (int i = 1; i <= logs_to_commit; i++) {
        std::string s = std::to_string(thread_id) + "_" + std::to_string(i) + "_" + std::to_string(rand());
        char const *log = s.c_str();
        std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 3));
        std::cout << "send to par_id " << par_id << std::endl;
        add_log_to_nc(log, strlen(log), par_id);
    }
}

// single paxos stream:
//   ./out-perf.masstree/benchmarks/paxos_async_commit_test -n 100 --num-of-partitions 3 -t 10 -F third-party/paxos/config/1silo_1paxos_2follower/3.yml -F third-party/paxos/config/occ_paxos.yml

int main(int argc, char **argv) {
    std::vector<std::string> paxos_config{};
    int logs_to_commit = 0, threads_num = 1;
    string paxos_proc_name = "localhost";
    int leader_config = 0;
    int multi_process = 0;
    int num_of_partitions = 1;

    static int countEnd = 0;

    while (1) {
        static struct option long_options[] = {
                {"paxos-leader-config", no_argument,       &leader_config, 1},
                {"multi-process",       no_argument,       &multi_process, 1},
                {"num-of-partitions",   required_argument, 0,              'p'},
                {0, 0,                                     0,              0}};

        int option_index = 0;
        int c = getopt_long(argc, argv, "P:F:n:t:p:", long_options, &option_index);
        if (c == -1)
            break;

        switch (c) {
            case 0:
                if (long_options[option_index].flag != 0)
                    break;
                abort();
                break;

            case 'n':
                logs_to_commit = strtoul(optarg, NULL, 10);
                break;

            case 'F':
                paxos_config.push_back(optarg);
                break;

            case 't':
                threads_num = strtoul(optarg, NULL, 10);
                break;

            case 'P':
                paxos_proc_name = string(optarg);
                break;

            case 'p':
                num_of_partitions = strtoul(optarg, NULL, 10);
                break;

            case '?':
                /* getopt_long already printed an error message. */
                exit(1);

            default:
                printf("Option not recognized");
                break;
        }
    }

    char *argv_paxos[18];
    argv_paxos[0] = (char *)"";
    argv_paxos[1] = (char *)"-b";
    argv_paxos[2] = (char *)"-d";
    argv_paxos[3] = (char *)"60";
    argv_paxos[4] = (char *)"-f";
    argv_paxos[5] = (char *) paxos_config[0].c_str();
    argv_paxos[6] = (char *)"-f";
    argv_paxos[7] = (char *) paxos_config[1].c_str();
    argv_paxos[8] = (char *)"-t";
    argv_paxos[9] = (char *)"30";
    argv_paxos[10] = (char *)"-T";
    argv_paxos[11] = (char *)"100000";
    argv_paxos[12] = (char *)"-n";
    argv_paxos[13] = (char *)"32";
    argv_paxos[14] = (char *)"-P";
    argv_paxos[15] = (char *) paxos_proc_name.c_str();
    argv_paxos[16] = (char *)"-A";
    argv_paxos[17] = (char *)"10000";  // bulkBatchCount
    int ret = setup(16, argv_paxos);
    if (ret != 0) {
        return ret;
    }

    int lCnt = 0, fCnt = 0;
    for (size_t i = 0; i < num_of_partitions; i++) {
        size_t par_id = i;

        register_for_leader_par_id_return([&lCnt](const char*& log, int len, int par_id, std::queue<std::tuple<unsigned long long int, int, int, const char *>> & un_replay_logs_) {
            lCnt++;
            return 0 ;
        }, par_id);

        register_for_follower_par_id_return([&fCnt](const char*& log, int len, int par_id, std::queue<std::tuple<unsigned long long int, int, int, const char *>> & un_replay_logs_) {
            if (len == 0) {
                countEnd++;
            }
            fCnt++;
            char *newLog = new char[len + 1];
            strcpy(newLog, log);
            std::cout << "received - value: " << newLog << ", par_id: " << par_id << std::endl;

            std::this_thread::sleep_for(std::chrono::milliseconds(rand() % 5));
            return 0 ;
        }, par_id);
    }

    setup2();

    if (leader_config || !multi_process) {
        std::vector<std::thread> threads(threads_num);
        for (int i = 0; i < threads_num; i++) {
            size_t par_id = i % num_of_partitions;
            threads[i] = std::thread(serialize_util_wrapper, logs_to_commit, i, par_id);
        }

        for (int i = 0; i < threads_num; i++) {
            threads[i].join();
        }

        for (int par_id = 0; par_id < num_of_partitions; par_id++) {
            std::string endS = "";
            char const *endLog = endS.c_str();
            add_log_to_nc(endLog, strlen(endLog), par_id);
        }
    }

    vector<std::thread> wait_threads;
    for (int par_id = 0; par_id < num_of_partitions; par_id++) {
        wait_threads.push_back(std::thread([par_id]() {
            std::cout << "Starting wait for " << par_id << std::endl;
            wait_for_submit(par_id);
        }));
    }
    for (auto &th : wait_threads) {
        th.join();
    }

    // should wait for a while for last piece of logs to be proceeded
    std::this_thread::sleep_for(std::chrono::seconds(1));

    if (!leader_config && multi_process) {
        while (countEnd < num_of_partitions) {
            std::cout << "received ending - " << countEnd << ", received msg - " << fCnt << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        };
    }

    pre_shutdown_step();
    ret = shutdown_paxos();

    std::cout << "Committed (follower) - " << fCnt << ", endReceived - " << countEnd << std::endl;
    return 0;
}