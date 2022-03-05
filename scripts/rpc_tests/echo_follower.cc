#include <iostream>
#include <thread>
#include <unistd.h>
#include "rpc/server.h"
#include "rpc/client.h"

using namespace std;

void follower_client() {
    rpc::client client("127.0.0.1", 8081);
    // start to send scanned logs
    string msg = "XXXXXXXXXXXXXXXXXXXXX" ;
    usleep(1 * 1000 * 1000) ;
    std::cout << "send the scanned logs: " << msg << std::endl;
    client.call("onScannedLogs", msg).as<int>();
    msg = "XXXXXXXXXXXXXXXXXXXXX-1" ;
    usleep(1 * 1000 * 1000) ;
    std::cout << "send the scanned logs: " << msg << std::endl;
    client.call("onScannedLogs", msg).as<int>();
    msg = "XXXXXXXXXXXXXXXXXXXXX-2" ;
    usleep(1 * 1000 * 1000) ;
    std::cout << "send the scanned logs: " << msg << std::endl;
    client.call("onScannedLogs", msg).as<int>();

    // start to send redo logs
    msg = "reXXXXXXXXXXXXXXXXXXXXX-0" ;
    usleep(1 * 1000 * 1000) ;
    std::cout << "send the redo logs: " << msg << std::endl;
    client.call("onRedoLogs", msg).as<int>();
    msg = "reXXXXXXXXXXXXXXXXXXXXX-1" ;
    usleep(1 * 1000 * 1000) ;
    std::cout << "send the redo logs: " << msg << std::endl;
    client.call("onRedoLogs", msg).as<int>();
    msg = "reXXXXXXXXXXXXXXXXXXXXX-2" ;
    usleep(1 * 1000 * 1000) ;
    std::cout << "send the redo logs: " << msg << std::endl;
    client.call("onRedoLogs", msg).as<int>();

    usleep(1 * 1000 * 1000) ;
    std::cout << "send the onLogDone" << std::endl;
    client.call("onLogDone").as<int>();
}

int onGetRole() {
    std::cout << "receive onGetRole" << std::endl;
    usleep(1 * 1000 * 1000) ;
    return 0 ;
}

int onSyncSource() {
    std::cout << "receive onSyncSource" << std::endl;

    thread start_follower_client(&follower_client);
    pthread_setname_np(start_follower_client.native_handle(), "start_follower_client");
    start_follower_client.detach();
    return 0;
}

int onAllDone() {
    std::cout << "receive onAllDone" << std::endl;
    usleep(1 * 1000 * 1000) ;

    // FLAG, start to kill all sub-threads
    return 0;
}

void server() {
    // Creating a server that listens on port 8080
    rpc::server srv(8080);

    // Binding the name "foo" to free function foo.
    // note: the signature is automatically captured
    srv.bind("onGetRole", &onGetRole);
    srv.bind("onSyncSource", &onSyncSource);
    srv.bind("onAllDone", &onAllDone);

    srv.run();
}

// follower_msg
int main(int argc, char *argv[]) {
    thread start_follower_server(&server);
    pthread_setname_np(start_follower_server.native_handle(), "start_follower_server");
    start_follower_server.detach();

    usleep(120 * 1000 * 1000) ;
    return 0;
}