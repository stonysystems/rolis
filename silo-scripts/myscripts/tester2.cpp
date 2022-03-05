//
// Created by weihshen on 6/27/21.
//
#include<algorithm>
#include<chrono>
#include<unistd.h>
#include <iostream>
using namespace std;

int main() {
    auto start = std::chrono::steady_clock::now();
    sleep(1);
    auto end = std::chrono::steady_clock::now();
    auto diff = end - start;
    auto timeTaken = std::chrono::duration <long double, std::milli> (diff).count() ;
    std::cout << timeTaken ;
    return 0 ;
}