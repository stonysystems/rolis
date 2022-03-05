//
// Created by weihshen on 1/5/21.
//
#include <iostream>
#include <string.h>
#include <chrono>
#include <thread>
#include <vector>
#include <stdlib.h>
#include <atomic>

using namespace std;

namespace sync_util {
    class sync_logger {
    public:
        static std::atomic<uint64_t> G_;  // G = min(cg_{0}, cg_{1}, cg_{2}, …, cg_{n})
        static vector<std::atomic<uint64_t>> cg_;
        static int thread_num;

        static void Init(int trd) {
            G_.store(0, memory_order_relaxed);
            for (int i = 0; i < 80; i++) {
                cg_[i].store(0, memory_order_relaxed);
            }
            thread_num = trd;
            thread advancer_thread(&sync_logger::advancer);
            pthread_setname_np(advancer_thread.native_handle(), "resto-advancer");  // XXX, why it doesn't work
            advancer_thread.detach();  // thread detach
        }

        static void advancer() {
            for (;;) {
                uint64_t min_so_far = numeric_limits<uint64_t>::max();

                for (int i = 0; i < thread_num; i++) {
                    min_so_far = min(min_so_far, cg_[i].load(memory_order_acquire));
                }

                G_.store(min_so_far, memory_order_release);
                std::cout << G_.load(memory_order_acquire) << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(100));  // 10 ms
                sync_util::sync_logger::advancer();
            }
        }
    };

    // initialize sync thread
    std::atomic<uint64_t> sync_logger::G_{0};
    vector<std::atomic<uint64_t>> sync_logger::cg_(80) ;
    int sync_logger::thread_num = 0;
}

void doJob(int tid) {
    for (int i=0; i<1000; i++) {
        std::this_thread::sleep_for (std::chrono::milliseconds (10));  // 10 ms
        auto latest_commit_id = rand() ;
        sync_util::sync_logger::cg_[tid].store(latest_commit_id, memory_order_release) ;
    }
}

int main() {
    auto trd_cnt = 4;
    std::vector<std::thread> threads(trd_cnt);

    // start a detached thread to advance G_ = min(cg_) and cg_ (latest_commit_id of paxos group)
    sync_util::sync_logger::Init(trd_cnt) ;

    for (int i = 0; i < trd_cnt; i++) {
        threads[i] = std::thread(doJob, i);
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cout << "Error calling pthread_setaffinity_np, code: " << rc << "\n";
        }
    }

    for (auto &t: threads)
        t.join() ;

    std::this_thread::sleep_for(std::chrono::seconds(1000));

    return 0 ;
}