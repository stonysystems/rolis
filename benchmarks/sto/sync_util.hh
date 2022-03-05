#ifndef _SYNC_UTIL_H
#define _SYNC_UTIL_H
#include <atomic>
#include <chrono>
#include <thread>
#include <mutex>
#include <condition_variable>

using namespace std;

namespace sync_util {
    class sync_logger {
    public:
        static std::string rpc_sync_host;
        static int rpc_sync_port;

        static std::atomic<uint64_t> G_;  // G = min(cg_{0}, cg_{1}, cg_{2}, …, cg_{n})
        static vector<std::atomic<uint64_t>> cg_;
        static int thread_num;
        static std::atomic<uint64_t> noops_cnt ;
        static std::atomic<uint64_t> noops_cnt_hole ;
        static std::atomic<uint64_t> noops_cnt_init ;
        static bool running ;

        // https://en.cppreference.com/w/cpp/thread/condition_variable
        static bool toLeader;
        static std::mutex m;
        static std::condition_variable cv;

        static void Init(int trd) {
            G_.store(0, memory_order_relaxed);
            for (int i = 0; i < trd; i++) {
                cg_[i].store(0, memory_order_relaxed);
            }
            thread_num = trd;
            thread advancer_thread(&sync_logger::advancer);
            pthread_setname_np(advancer_thread.native_handle(), "resto-advancer");
            advancer_thread.detach();  // thread detach
        }

        static void shutdown() {
            running = false ;
            toLeader = true ; // in order to stop the thread in the follower
            sync_util::sync_logger::cv.notify_one();
        }

        static void output() {
            std::cout << "-------" << std::endl;
            for (int i = 0; i < thread_num; i++) {
                std::cout << "  ----par_id" << i << ", cg_: " << cg_[i].load(memory_order_acquire) << std::endl;
            }
        }

        static uint64_t computeG() { // compute G immediately and strictly
            uint64_t min_so_far = numeric_limits<uint64_t>::max();

            for (int i = 0; i < thread_num; i++) {
                auto c = cg_[i].load(memory_order_acquire) ;
                min_so_far = min(min_so_far, c);
            }
            G_.store(min_so_far, memory_order_release);
            return G_.load(memory_order_acquire) ;  // invalidate the cache
        }

        static uint64_t retrieveG(bool strict=false) {
            //return G_.load(strict? memory_order_acquire : memory_order_relaxed);
            return G_.load(memory_order_acquire);
        }

        static void advancer() {
            while(running) {
                uint64_t min_so_far = numeric_limits<uint64_t>::max();

                for (int i = 0; i < thread_num; i++) {
                    auto c = cg_[i].load(memory_order_acquire) ;
                    if (c > 0)
                        min_so_far = min(min_so_far, cg_[i].load(memory_order_acquire));
                }
                G_.store(min_so_far, memory_order_release);
                std::this_thread::sleep_for(std::chrono::microseconds(5000));  // 5 ms
                sync_util::sync_logger::advancer();
            }
        }
    };

    std::string sync_logger::rpc_sync_host = "";
    int sync_logger::rpc_sync_port = 0;

    // initialize sync thread
    std::atomic<uint64_t> sync_logger::G_{0};
    std::atomic<uint64_t> sync_logger::noops_cnt{0};
    std::atomic<uint64_t> sync_logger::noops_cnt_hole{0};
    std::atomic<uint64_t> sync_logger::noops_cnt_init{0};
    vector<std::atomic<uint64_t>> sync_logger::cg_(80) ;  // maximal size of threads
    int sync_logger::thread_num = 0;
    bool sync_logger::running = true;
    bool sync_logger::toLeader = false ;
    std::mutex sync_logger::m ;
    std::condition_variable sync_logger::cv ;
}

#endif
