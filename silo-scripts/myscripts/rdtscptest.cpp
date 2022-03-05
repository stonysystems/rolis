//
// Created by weihshen on 1/30/21.
//
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <x86intrin.h>

using namespace std;

void test(int tid) {
    std::this_thread::sleep_for (std::chrono::seconds (tid));
    unsigned int ui;
    uint64_t tid_unique_ = __rdtscp(&ui);
    std::cout << "tid: " << tid << ", counter: " << tid_unique_ << ", ui: " << ui << std::endl;
    std::this_thread::sleep_for (std::chrono::seconds (1));
}

int main() {
    size_t trd_cnt = 3 ;
    std::vector<std::thread> threads(trd_cnt);

    for (size_t i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(test, i);  // three threads with tid: 0, 1, 2
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cout << "Error calling pthread_setaffinity_np, code: " << rc << "\n";
        }
    }

    for (size_t i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }

    return 0;
}