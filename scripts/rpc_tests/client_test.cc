#include <iostream>
#include <thread>
#include <unistd.h>
#include "rpc/server.h"
#include "rpc/client.h"

using namespace std;

int main(int argc, char *argv[]) {
    rpc::client client("127.0.0.1", 8081);
    std::cout << "get a response - onGetRole: " << client.call("onGetRole").as<int>() << " to 8081" << std::endl;

    std::cout << "get a response - onSyncSource: " << client.call("onSyncSource").as<int>() << std::endl;

    rpc::client client2("127.0.0.1", 8082);
    std::cout << "get a response - onGetRole: " << client2.call("onGetRole").as<int>() << " to 8082" << std::endl;

    usleep(120 * 1000 * 1000) ;
    return 0;
}

// g++ client_test.cc -g -lrpc -lpthread -Wall -O0 -o client_test