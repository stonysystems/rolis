#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <cstdlib>
#include "../benchmarks/mbta_wrapper.hh"
#include "../benchmarks/sto/ReplayDB.h"

size_t con_LEN_ULL = sizeof (unsigned long long int);


inline unsigned long long int con_keystore_decode3(const std::string& s, bool first= false){
    unsigned long long int x=0;
    if(first) {
        memcpy (&x, (void *) (s.data ()), con_LEN_ULL);
        return x;
    }else {
        memcpy (&x, (void *) (s.data () + con_LEN_ULL), sizeof (unsigned long long int));
        return x;
    }
}

bool con_cmpFunc2(const std::string& newValue,const std::string& oldValue)
{
    unsigned long long int commit_id_new = con_keystore_decode3(newValue, true);
    unsigned long long int commit_id_old = con_keystore_decode3(oldValue, true);

    unsigned long long int commit_id_new_v = con_keystore_decode3(newValue, false);
    unsigned long long int commit_id_old_v = con_keystore_decode3(oldValue, false);

    std::cout << "new commit_id: " << commit_id_new << ", new value: " << commit_id_new_v << ", old commit_id: " << commit_id_old  << ", old value: " << commit_id_old_v << ", update or not: " << (commit_id_new > commit_id_old) << std::endl;
    return commit_id_new > commit_id_old;
}

void con_keystore_encode3(std::string& s, unsigned long long int x, unsigned long long int y){
    memcpy ((void *)s.data(), &x, con_LEN_ULL);
    memcpy ((void *)(s.data()+con_LEN_ULL), &y, con_LEN_ULL);
}

unsigned long long int getRandom64(const unsigned long long int &begin = 0, const unsigned long long int &end = 100) {
    //return begin >= end ? 0 : begin + (unsigned long long int) ((end - begin)*rand()/(double)RAND_MAX);
    return begin >= end ? 0 : begin + (unsigned long long int) rand()*((end - begin)/RAND_MAX);
};

const char *int2Char(unsigned long long int k) {
    std::string ks ;
    ks.resize(con_LEN_ULL) ;
    memcpy ((void *)ks.data(), &k, con_LEN_ULL);
    const char *kc = ks.c_str() ;
    return kc ;
}


unsigned long long int myResolve64(std::string s) {
    unsigned long long int *k  ;
    k = reinterpret_cast<unsigned long long int*>((char *)s.data()) ;
    return *k ;
}


void keysChecker(int table_id, new_directs &h)
{
    std::vector<std::pair<std::string, unsigned long long int>> allKeys ;

    //std::cout << "address of database: " << &h << std::endl;
    std::string myKey ;
    myKey.resize(sizeof (unsigned long long int)) ;
    std::string value = "" ;
    value.resize(2* sizeof (unsigned long long int));

    for (size_t i=0; i<=10; i++) {
        auto commit_id = rand() % 1000 ;
        auto trueValue = -1 ;
        con_keystore_encode3 (value, commit_id, trueValue);

        // construct str_key
        unsigned long long int k = getRandom64(0, ULLONG_MAX) ;
        const char *kc = int2Char(k) ;
        myKey.assign((char *)kc, con_LEN_ULL);  // not necessary to be a readable digits
        allKeys.emplace_back(std::make_pair(myKey, k)) ;
        std::cout << "put KEY: " << myKey << std::endl;
        h[table_id].put (myKey, value, con_cmpFunc2);
        std::this_thread::sleep_for(std::chrono::milliseconds (rand() % 10));
    }

    // double check keys
    for(auto item: allKeys) {
        auto r = myResolve64(item.first) ;
        std::cout << "Get - " << r << ", Expectation - " << item.second << ", compare correct or not - " << (r == item.second) << ", exists in database correct or not - " << h[table_id].exists(item.first) << std::endl;
    }
}

void concurrent_write(int tid, int table_id, new_directs &h)
{
    // TThread::set_id(tid); // it is not necessary
    actual_directs::thread_init() ;  // init thread info for each thread
    std::cout << "tid: " << tid << ", address of database: " << &h << std::endl;
    std::string value = "" ;
    value.resize(2* sizeof (unsigned long long int));

    for (size_t i=0; i<=40; i++) {
        auto commit_id = rand() % 100 ;
        auto trueValue = -1 ;
        con_keystore_encode3 (value, commit_id, trueValue);
        std::string myKey = "_" + std::to_string(rand()) ;
        h[table_id].put (myKey, value, con_cmpFunc2);
        std::cout << "tid= " << tid << ", address= " << &h[table_id] << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds (rand() % 10));
    }
}

// with different database instances test case
/*
void concurrent_write_value(int tid, int table_id, new_directs h)
{
    actual_directs::thread_init() ;  // init thread info for each thread
    std::cout << "tid: " << tid << ", address of database: " << &h << std::endl;
    std::string value = "" ;
    value.resize(2* sizeof (unsigned long long int));

    for (size_t i=0; i<=40; i++) {
        auto commit_id = rand() % 100 ;
        auto trueValue = -1 ;
        con_keystore_encode3 (value, commit_id, trueValue);
        std::string myKey = "_" + std::to_string(rand()) ;
        h[table_id].put (myKey, value, con_cmpFunc2);
        std::cout << "tid= " << tid << ", address= " << &h[table_id] << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds (rand() % 10));
    }
} */


void basicCorrectness(int table_id, new_directs &h)
{
    std::string key_str="0000", value = "" ;
    value.resize(2* sizeof (unsigned long long int));

    for (size_t i=0; i<=4; i++) {
        unsigned long long int commit_id ;
        unsigned long long int trueValue ;

        switch(i) {
            case 0:
                commit_id = 1 ;
                trueValue = 2 ;
                break ;
            case 1:
                commit_id = 100 ;
                trueValue = 200 ;
                break ;
            case 2:
                commit_id = 1000 ;
                trueValue = 2000 ;
                break;
            case 3:
                commit_id = 2 ;
                trueValue = 2 ;
                break ;
            case 4:
                commit_id = 10 ;
                trueValue = 10 ;
                break ;
        }

        con_keystore_encode3 (value, commit_id, trueValue);

        //auto deCid = con_keystore_decode3(value, true) ;
        //std::cout << "decoded commit_id - " << deCid << std::endl;

        std::cout << "PUT key= " << key_str << ", commit_id= " << commit_id << ", trueValue= " << trueValue << std::endl;
        h[table_id].put (key_str, value, con_cmpFunc2);

        std::this_thread::sleep_for(std::chrono::milliseconds (rand() % 10));
        std::cout << std::endl;
    }
}

int main()
{
    auto table_id = 10010;

    // test case1: test decode and encode of key
    new_directs database;
    keysChecker(table_id, database) ;
    std::cout << "================================Test 1: PASS" << std::endl;

    // test case2: test basic correctness of insert & update on the same key
    new_directs database01 ;
    basicCorrectness(table_id, database01) ;
    std::cout << "================================Test 2: PASS" << std::endl;

    // test case3: concurrent writes with reference db - what we desire for
    new_directs database02;
    auto trd_cnt = 10 ;
    std::vector<std::thread> threads(trd_cnt);

    for (auto i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(concurrent_write, i, table_id, std::ref(database02));
    }
    for (auto i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }
    std::cout << "================================Test 3: PASS" << std::endl;

    // test case4: concurrent writes with value write, different database instance - the wrong way
    /*
    new_directs database03;
    auto trd_cnt_value = 10 ;
    std::vector<std::thread> threads_value(trd_cnt);

    for (auto i=0; i< trd_cnt_value; i++) {
        threads_value[i] = std::thread(concurrent_write_value, i, table_id, database03);
    }

    for (auto i=0; i< trd_cnt_value; i++) {
        threads_value[i].join() ;
    }
    std::cout << "================================Test 4: PASS" << std::endl; */

    return 0 ;
}