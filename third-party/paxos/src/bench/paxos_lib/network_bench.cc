#include "../../deptran/s_main.h"
#include <iostream>
#include <vector>
#include <sys/time.h>
#include <thread>
#include <string>
#include <cstring>


int setup_paxos_default(){
  std::vector<std::string> paxos_config{"config/1c1s1p.yml", "config/occ_paxos.yml"};
  char *argv_paxos[16];
  argv_paxos[0] = "build/microbench";
  argv_paxos[1] = "-b";
  argv_paxos[2] = "-d";
  argv_paxos[3] = "60";
  argv_paxos[4] = "-f";
  argv_paxos[5] = (char *) paxos_config[0].c_str();
  argv_paxos[6] = "-f";
  argv_paxos[7] = (char *) paxos_config[1].c_str();
  argv_paxos[8] = "-t";
  argv_paxos[9] = "30";
  argv_paxos[10] = "-T";
  argv_paxos[11] = "100000";
  argv_paxos[12] = "-n";
  argv_paxos[13] = "32";
  argv_paxos[14] = "-P";
  argv_paxos[15] = "localhost";
  std::vector<std::string> ret = setup(16, argv_paxos);
  register_for_leader([](const char* log, int len) {
  }, 0);
  register_for_follower([](const char* log, int len) {
  }, 0);
  int ret2 = setup2();
  if (ret.empty() || ret2 != 0) {
    exit(-1);
  }
  return 0;
}

int shutdown(){
  pre_shutdown_step();
  int ret = shutdown_paxos();
  if(ret != 0){
    exit(-1);
  }
}

int main(int argc, char* argv[]){
  setup_paxos_default();
  int times = 1;
  std::string s = std::to_string(times);
  char *log = (char*)s.c_str(); //simulating batching in paxos
  for(int i = 0; i < times; i++){
    add_log_to_nc(log, (int)s.size(), 0);
    //std::this_thread::sleep_for(std::chrono::nanoseconds(1));
  }
  wait_for_submit(0);
  shutdown();
}
