//
// Created by mrityunjay kumar on 2019-07-13.
//
#ifndef FUNC_POOL_CPP
#define FUNC_POOL_CPP
#include "function_pool.h"

#ifndef MASS_DIRECT

//Function_pool::Function_pool() : m_function_queue(), m_lock(), m_data_condition(), m_accept_functions(true)
//{
//}
//
//Function_pool::~Function_pool()
//{
//}
//
//void Function_pool::push(const std::function<void(ds &,const container_hello*)>& func, ds &h, const container_hello* filename)
//{
//    std::unique_lock<std::mutex> lock(m_lock);
//    m_function_queue.push(std::make_pair (func, std::make_pair (h,filename)));
//    // when we send the notification immediately, the consumer will try to get the lock , so unlock asap
//    lock.unlock();
//    m_data_condition.notify_all();
//}
//
//void Function_pool::done()
//{
////    std::cout << "------------" << std::endl;
//    std::unique_lock<std::mutex> lock(m_lock);
//    m_accept_functions = false;
//    lock.unlock();
//    // when we send the notification immediately, the consumer will try to get the lock , so unlock asap
//    m_data_condition.notify_all();
//    //notify all waiting threads.
////    std::cout << "------------" << std::endl;
//}
//
//void Function_pool::infinite_loop_func()
//{
////  std::pair<std::function<void(ds, std::string)>,std::pair<ds,std::string> > top;
////    std::function<void(ds, std::string)> func;
//
//    std::pair<std::function<void(ds&,const container_hello*)>,std::pair<ds,const container_hello*> > top;
//    std::function<void(ds&,const container_hello*)> func;
//    while (true)
//    {
//        {
//            std::unique_lock<std::mutex> lock(m_lock);
//            m_data_condition.wait(lock, [this]() {return !m_function_queue.empty() || !m_accept_functions; });
//            if (!m_accept_functions && m_function_queue.empty())
//            {
//                //lock will be release automatically.
//                //finish the thread loop and let it join in the main thread.
//                return;
//            }
//            top = m_function_queue.front();
//            m_function_queue.pop();
//            //release the lock
//        }
//        func = top.first;
//        func(top.second.first, top.second.second);
//    }
//}
#endif
#endif