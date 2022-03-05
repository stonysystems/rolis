//
// Created by weihshen on 9/11/21.
//
#include <iostream>
#include <cstring>
#include <random>
#include <memory>

using namespace std;

int
main(int argc, char **argv)
{
    // clang-format off
    const char *rte_argv[] = {
            "-c",            "0x0",
            "-n",            "6",  // Memory channels
            "-m",            "1024", // Max memory in megabytes
            "--proc-type",   "auto",
            nullptr};
    // clang-format on

    const int rte_argc =
            static_cast<int>(sizeof(rte_argv) / sizeof(rte_argv[0])) - 1;
    std::cout << argc << std::endl;

    return 0;
}
