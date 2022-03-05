#include "deptran/s_main.h"
#include <iostream>
#include <assert.h>
#include<cstring>
#include <stdio.h>
#include <thread>
#include <vector>
#include <iostream>
#include <queue>
#include <sstream>
#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>
#include <exception>
#include <fstream>
#include <thread>
#include <vector>
#include <iostream>
#include <thread>
#include <mutex>
#include <functional>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <condition_variable>
#include <glob.h>
#include <sys/time.h>

int main(int argc, char* argv[]) {
  std::vector<std::string> paxos_config{"config/1c1s1p.yml", "config/occ_paxos.yml"};
  int logs_to_commit = 1000000, c;
  char *argv_paxos[16];
  //HeapProfilerStart("abc");
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
  int ret2 = setup2();
  if (ret.empty() || ret2 != 0) {
    return 1;
  }
  int cnt = 0;
  register_for_leader([&cnt](const char* log, int len) {
    cnt++;
    //std::cout << "committed" << std::endl;
  }, 0);
  register_for_follower([](const char* log, int len) {
  }, 0);
  char *x = (char *)malloc(4*1024);
  memset(x, 'a', 4*1024);
  for(int i = 1; i <= logs_to_commit; i++){
    add_log(x , strlen(x), 0);
    //wait_for_submit(0);
    //std::cout << cnt << std::endl;
  }
  wait_for_submit(0);
  //HeapProfilerDump("abc");
  //HeapProfilerStop();
  while(true){}
  pre_shutdown_step();
  ret2 = shutdown_paxos();
  //while(true){}
  std::cout << "Submitted " << logs_to_commit << std::endl;
  std::cout << "Committed " << cnt << std::endl;
//  // microbench_paxos();
//  microbench_paxos_queue();
  return ret2;
}
