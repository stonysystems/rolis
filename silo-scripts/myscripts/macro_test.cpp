//
// Created by weihshen on 11/18/2021.
//

// https://riptutorial.com/cplusplus/example/3527/macros
#include <iostream>
#include <cstring>
#include <random>
#include <memory>

using namespace std;

int var_1 = 1;
int var_2 = 2;
#define SHOW(a) std::cout << #a << ": " << (a) << std::endl

int main(int argc, char **argv)
{
    SHOW(var_1);
    SHOW("var_1");
    return 0;
}