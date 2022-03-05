//
// Created by weihshen on 2/6/21.
//
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <cstdlib>

using namespace std;

// atomic load and atomic store operations: https://riptutorial.com/cplusplus/example/25795/need-for-memory-model
// greate example: https://stackoverflow.com/questions/13632344/understanding-c11-memory-fences
// Synchronization means anything that happens-before the release fence happens-before anything that happens-after the acquire fence,
// but without atomic (memory_order_relaxed), written value might not be visible to others

// bacon algorithms, no-locks
// derived an example from this:

int main() {
#ifndef MASS_DIRECT
#define MASS_DIRECT
    std::cout << "XXXX" << std::endl;
#endif
    std::cout << "YYY" << std::endl;
    return 0 ;
}