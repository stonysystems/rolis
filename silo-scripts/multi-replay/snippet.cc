
#include <chrono>
#include <thread>

void sleepB(std::string prev, int m, std::string msg) {
    if (prev != "")
        std::cout << prev << " running on coreid " << sched_getcpu() << std::endl;
    std::this_thread::sleep_for (std::chrono::seconds (m));
    if (msg != "")
        std::cout << msg << " running on coreid " << sched_getcpu() << std::endl;
}

register_for_follower([=](const char* log, int len) {
    std::ofstream outfile("log/follower_dbtest.txt", ios::app);
    outfile << log << std::endl;
    outfile.close();
  } i);
