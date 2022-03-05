//
// Created by weihshen on 2/13/21.
//

#include <iostream>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <queue>
#include <thread>
#include <ctime>
#include <string.h>
#include <chrono>
#include <tuple>
using namespace std;

void change(std::queue<std::pair<unsigned long long int, const char *>> &) ;
void change(std::queue<std::pair<unsigned long long int, const char *>> &b) {
    const char *test = "111" ;
    std::cout << "inside: " << &test << std::endl;
    b.push(std::make_pair(1, test)) ;
}

void change_xx(std::queue<std::tuple<unsigned long long int, const char *, int, int>> &b) {
    const char *test = "111" ;
    b.push(std::make_tuple(122, test, 100, 200)) ;
    auto it_xx = b.front() ;
    const char *c = std::get<1>(it_xx) ;
    std::cout << "inside2: " << &c << "_" << c << std::endl;
}

int main() {
//    std::queue<std::pair<unsigned long long int, const char *>> un_replay_logs_ ;
//    std::cout << &un_replay_logs_ << std::endl;
//    change(un_replay_logs_) ;
//    std::cout << un_replay_logs_.size() << std::endl;
//
//    auto it = un_replay_logs_.front() ;
//    std::cout << it.first << " : " << it.second << std::endl;
//    std::cout << "outside: " << &it.second << std::endl;

//    unsigned long long int x = 234813483194005 ;
//    std::cout << x << std::endl;

    std::queue<std::tuple<unsigned long long int, const char *, int, int>> un_replay_logs_xx ;
    std::string cx = "-00000" ;
    const char *log = cx.c_str() ;
    char *cclog = (char *)malloc(10) ;
    memcpy(cclog, log, 10) ;
    un_replay_logs_xx.push(std::make_tuple(122, (const char*)cclog, 100, 200)) ;
    //change_xx(un_replay_logs_xx) ;
    auto it_xx = un_replay_logs_xx.front() ;
    const char *c = std::get<1>(it_xx) ;
    std::cout << "outside2: " << &c << "_" << c << std::endl;

//    auto time_stamp = std::to_string(static_cast<long int> (std::time(0)));
//    std::cout << time_stamp << std::endl;
//
//    std::this_thread::sleep_for (std::chrono::seconds (1));
//
//    std::cout << std::to_string(static_cast<long int> (std::time(0))) << std::endl;
}