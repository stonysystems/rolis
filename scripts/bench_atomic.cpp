//
// Created by weihshen on 2/10/21.
//

#include <iostream>
#include <atomic>
#include <vector>
#include <x86intrin.h>
#include <chrono>
#include <thread>

std::atomic<long long> f_cnt;
const int N = 50000000 ;

void do_work(int tid) {
    while (1) {
        f_cnt.fetch_add(1, std::memory_order_relaxed);
        if (f_cnt > N) {
            break ;
        }
    }
    //std::cout << "tid " << tid << " is ended." << std::endl; 
}

uint64_t r_cnt = 0;
uint64_t x = 0;

void do_work_rdtscp(int cnt, int tid) {
    for (auto i=0; i<int(N/cnt); i++) {
        unsigned int ui ;
        r_cnt = __rdtscp(&ui);
    }
    //std::cout << "tid " << tid << " is ended." << std::endl; 
}


void test_fetch_add(int trd_cnt) {
    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads(trd_cnt);

    for (int i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(do_work, i);
	cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    for (int i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }

    auto end = std::chrono::steady_clock::now();
    auto s = std::chrono::duration <long double, std::milli> (end - start).count() / 1000.0;
    //std::cout << "[fetch_add] # of operation:" << f_cnt << ", elapsed: " << s << " sec, avg: " << f_cnt / float(s) / float(trd_cnt) << " per thread per sec" << '\n';
    std::cout << f_cnt / float(s) / float(trd_cnt);

}

void test_rdtscp(int trd_cnt) {
    auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> threads(trd_cnt);

    for (int i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(do_work_rdtscp, trd_cnt, i);
	cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);
        int rc = pthread_setaffinity_np(threads[i].native_handle(),
                                        sizeof(cpu_set_t), &cpuset);
        if (rc != 0) {
            std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
        }
    }

    for (int i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }

    auto end = std::chrono::steady_clock::now();
    auto s = std::chrono::duration <long double, std::milli> (end - start).count() / 1000.0;
    //std::cout << "[rdtscp] # of operation:" << N << ", elapsed: " << s << " sec, avg: " << N / float(s) / float(trd_cnt) << " per thread per sec" << '\n';
    std::cout << N / float(s) / float(trd_cnt);

}

int main(int argc, char *argv[]) {
    auto trd_cnt = std::stoi(argv[1]);
    std::string bench = std::string(argv[2]);
    //std::cout << "----- results for threads: " << trd_cnt << std::endl;
    if (bench.compare("fetch") == 0) {
        test_fetch_add(trd_cnt) ;
    	std::this_thread::sleep_for (std::chrono::seconds (3)); // sleep 3 second to cool down the machine
    }

    if (bench.compare("rdtscp") == 0) {
    	test_rdtscp(trd_cnt) ;
    	std::this_thread::sleep_for (std::chrono::seconds (3)); // sleep 3 second to cool down the machine
    }
    std::cout << "\navoid optimization: " << r_cnt << " : " << f_cnt; // avoid optimized out 
}
