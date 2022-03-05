#include <iostream>
#include <unistd.h>
#include "rpc/server.h"
#include "rpc/client.h"

using namespace std;

int onScannedLogs(string logs) {
    std::cout << "receive a onScannedLogs: " << logs << std::endl;
    usleep(1 * 1000 * 1000) ;
    return 0 ;
}

int onRedoLogs(string logs) {
    std::cout << "receive a onRedoLogs: " << logs << std::endl;
    usleep(1 * 1000 * 1000) ;
    return 0 ;
}

void afterLogDONE() {
    std::cout << "do the stuff after logDONE...\n" ;
    usleep(3 * 1000 * 1000) ;

    // then send "all_DONE" to the follower
    rpc::client client("127.0.0.1", 8080);
    std::cout << "send onAllDone...\n" ;
    auto result = client.call("onAllDone").as<int>();

    // FLAG, start to kill all sub-threads
}

int onLogDone() {
    std::cout << "receive a onLogDone" << std::endl;
    usleep(1 * 1000 * 1000) ;
    thread start_thread(&afterLogDONE);
    pthread_setname_np(start_thread.native_handle(), "afterLogDONE");
    start_thread.detach();
    return 0 ;
}

void older_leader_server() {
    // Creating a server that listens on port 8081
    rpc::server srv(8081);

    srv.bind("onScannedLogs", &onScannedLogs);
    srv.bind("onRedoLogs", &onRedoLogs);
    srv.bind("onLogDone", &onLogDone);

    srv.run();
}

// old leader
int main() {
    // start a server to receive logs from follower
    thread start_old_leader_server(&older_leader_server);
    pthread_setname_np(start_old_leader_server.native_handle(), "start_old_leader_server");
    start_old_leader_server.detach();

    // Creating a client that connects to the localhost on port 8080
    rpc::client client("127.0.0.1", 8080);
    std::cout << "send onGetRole...\n" ;
    auto result = client.call("onGetRole").as<int>();

    std::cout << "send onSyncSource...\n" ;
    result = client.call("onSyncSource").as<int>();

    usleep(120 * 1000 * 1000) ;
    return 0;
}