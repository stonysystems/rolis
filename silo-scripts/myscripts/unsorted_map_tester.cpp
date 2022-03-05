//
// Created by weihshen on 1/30/21.
//
#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <string.h>
#include <random>
#include <vector>
#include <unordered_map>

using namespace std;

std::unordered_map<long, long> len_counter ;

void test() {
    for (int i=0; i<=10000; i++) {
        len_counter[rand() % 10] += 1;
        std::this_thread::sleep_for(std::chrono::microseconds(1000));
    }
}

int main() {
    size_t trd_cnt = 10 ;
    std::vector<std::thread> threads(trd_cnt);

    for (size_t i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(test);
    }

    for (size_t i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }

    long sum = 0;
    for (auto i: len_counter) {
        printf("K: %ld, v: %ld\n", i.first, i.second) ;
        sum += i.second ;
    }
    // 10000 * 10 * 10 = 1M
    printf(" -- sum: %ld\n", sum) ;
    return 0;
}