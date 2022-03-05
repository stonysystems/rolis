#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <functional>
#include <chrono>
#include <atomic>

class SharedThreadPool
{
public:

    SharedThreadPool (int threads) : shutdown_ (false)
    {
        // Create the specified number of threads
        threads_.reserve (threads);
        for (int i = 0; i < threads; ++i)
            threads_.emplace_back (std::bind (&SharedThreadPool::threadEntry, this, i));
    }

    ~SharedThreadPool () { }

    void joinAll() {
        {
            // Unblock any threads and tell them to stop
            std::unique_lock <std::mutex> l (lock_);

            shutdown_ = true;
            condVar_.notify_all();
        }

        // Wait for all threads to stop
        // std::cerr << "JOIN threads" << std::endl;
        for (auto& thread : threads_)
            thread.join();
    }

    void doJob (std::function <void (void)> func)
    {
        tasks ++ ;
        // Place a job on the queue and unblock a thread
        std::unique_lock <std::mutex> l (lock_);

        jobs_.emplace (std::move (func));
        condVar_.notify_one();
    }

    size_t getNotRunningSize() {
        return jobs_.size() ;
    }

    void waitDone() {
        while (tasks != done) {};
        done = 0 ;
        tasks = 0 ;
    }

    int getDone() {
        return done ;
    }

    int getTasks() {
        return tasks;
    }

protected:

    void threadEntry (int i)
    {
        std::function <void (void)> job;

        while (1)
        {
            {
                std::unique_lock <std::mutex> l (lock_);

                while (! shutdown_ && jobs_.empty()) {
                    condVar_.wait (l);
                }

                if (jobs_.empty ())
                {
                    // No jobs to do and we are shutting down
                    std::cerr << "Thread " << i << " terminates" << std::endl;
                    return;
                }

                job = std::move (jobs_.front ());
                jobs_.pop();

            }
            // Do the job without holding any locks
            job ();
            done ++ ;
        }

    }

    std::mutex lock_;
    std::condition_variable condVar_;
    bool shutdown_;
    std::queue <std::function <void (void)>> jobs_;
    std::vector <std::thread> threads_;
    std::atomic<int> done{0};
    std::atomic<int> tasks{0};
};



void silly (size_t i, int n)
{
    // A silly job for demonstration purposes
    //std::cerr << "starting, task_id " << i << " and sleep for " << n << " seconds, coreid " << sched_getcpu() <<  std::endl;
    std::this_thread::sleep_for (std::chrono::seconds (n));
    //std::cerr << "done, task_id " << i << " and sleep for " << n << " seconds, coreid " << sched_getcpu() <<  std::endl;
}

int main()
{
    // Create two threads
    SharedThreadPool p (3);

//    for (size_t i=0;i<101;i++) {
//        p.doJob(std::bind (silly, i, 1)) ;
//    }
//    p.waitDone() ;
//    std::cout << "1. not running is " << p.getNotRunningSize() << ", done is " << p.getDone() << std::endl;

    for (size_t i=0;i<5;i++) {
        p.doJob(std::bind (silly, i, i % 4 + 1)) ;
    }
    std::cout << "t: " << p.getTasks() << ", done: " << p.getDone() << std::endl;
    p.waitDone() ;
    std::cout << "2. not running is " << p.getNotRunningSize() << ", done is " << p.getDone() << std::endl;

//    std::this_thread::sleep_for (std::chrono::seconds (10));
//    for (size_t i=0;i<105;i++) {
//        p.doJob(std::bind (silly, i, i % 4 + 1)) ;
//    }
//    p.waitDone() ;
//    std::cout << "3. not running is " << p.getNotRunningSize() << ", done is " << p.getDone() << std::endl;
//
//    std::this_thread::sleep_for (std::chrono::seconds (10));
//    for (size_t i=0;i<4;i++) {
//        p.doJob(std::bind (silly, i, i % 4 + 1)) ;
//    }
//    p.waitDone() ;
//    std::cout << "4. not running is " << p.getNotRunningSize() << ", done is " << p.getDone() << std::endl;

    p.joinAll() ;

//    std::cout << "NOT running tasks " << p.getSize() << std::endl ;
//
//    while (p.getSize() > 0) {
//        std::cout << "NOT running tasks " << p.getSize() << std::endl;
//        std::this_thread::sleep_for (std::chrono::milliseconds(100));
//    }
}