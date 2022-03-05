#include <iostream>
#include "macros.h"
#include "thread.h"

using namespace std;

ndb_thread::~ndb_thread()
{
}

void
ndb_thread::start()
{
  thd_ = std::move(thread(&ndb_thread::run, this));
  if (daemon_)
    thd_.detach();
}

void
ndb_thread::checkaffinity() {
  cpu_set_t cpuset;
  int rc = pthread_getaffinity_np(thd_.native_handle(), sizeof(cpu_set_t), &cpuset);
  if (rc != 0)
      std::cout << "Error calling pthread_getaffinity_np, code: " << rc << "\n";

  for (size_t j = 0; j < CPU_SETSIZE; j++)
      if (CPU_ISSET(j, &cpuset))
          printf("    assigned CPU %ld\n", j);
}

void
ndb_thread::start(int cpu_id)
{
  thd_ = std::move(thread(&ndb_thread::run, this));

  cpu_set_t cpuset;
  CPU_ZERO(&cpuset);
  CPU_SET(cpu_id, &cpuset);
  int rc = pthread_setaffinity_np(thd_.native_handle(),
			 sizeof(cpu_set_t), &cpuset);
  if (rc != 0) {
      std::cout << "Error calling pthread_setaffinity_np, code: " << rc << "\n";
  } 

  if (daemon_)
    thd_.detach();
}

void
ndb_thread::join()
{
  ALWAYS_ASSERT(!daemon_);
  thd_.join();
}

// can be overloaded by subclasses
void
ndb_thread::run()
{
  ALWAYS_ASSERT(body_);
  body_();
}
