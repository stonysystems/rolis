#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <algorithm>
#include <atomic>
#include <vector>
#include <x86intrin.h>
#include "../benchmarks/mbta_wrapper.hh"
#include "../benchmarks/sto/ReplayDB.h"
#include "../str_arena.h"
#include <limits.h>

size_t con_LEN_ULL = sizeof (unsigned long long int);
std::atomic<long long> f_cnt;

mbta_wrapper *ndb = new mbta_wrapper;
abstract_ordered_index *user_table_global = ndb->open_index("user_0", 1, false, false) ;  // mbta_ordered_index

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

void con_keystore_encode3(std::string& s, unsigned long long int x, unsigned long long int y){
    memcpy ((void *)s.data(), &x, con_LEN_ULL);
    memcpy ((void *)(s.data()+con_LEN_ULL), &y, con_LEN_ULL);
}

bool con_cmpFunc2(const std::string& newValue,const std::string& oldValue)
{
    unsigned long long int commit_id_new = con_keystore_decode3(newValue, true);
    unsigned long long int commit_id_old = con_keystore_decode3(oldValue, true);

    unsigned long long int commit_id_new_v = con_keystore_decode3(newValue, false);
    unsigned long long int commit_id_old_v = con_keystore_decode3(oldValue, false);

    std::cout << "new commit_id: " << commit_id_new
             << ", new value: " << commit_id_new_v
             << ", old commit_id: " << commit_id_old
             << ", old value: " << commit_id_old_v
             << ", update or not: " << (commit_id_new > commit_id_old) << std::endl;
    return commit_id_new > commit_id_old;
}

void concurrent_write_direct(int tid, int nums, int table_id, new_directs &h)
{  // jay's implementation, discard
    TThread::set_id(tid + 1);
    actual_directs::thread_init() ;  // init thread info for each thread
    std::string value = "111111" ;
    ofstream key_file("./scripts/keys_" + std::to_string(tid)) ;

    for (int j=0; j<nums; j++) {
        unsigned int ui ;
        uint64_t commit_id = __rdtscp(&ui);
        std::string myKey = std::to_string(commit_id * 100 + tid) ;
        key_file << myKey ;
        key_file << "\n" ;
        h[table_id].put (myKey, value, con_cmpFunc2);

        if (!h[table_id].exists(myKey)) {
            std::cout << "not found - immediately - direct: " << myKey << std::endl;
        }
    }
    key_file.close() ;
}

class new_scan_callback_bench : public abstract_ordered_index::scan_callback {
public:
    new_scan_callback_bench() {
    }
    bool invoke(
            const char *keyp, size_t keylen,
            const string &value) override
    {
        if (values.empty() && skip_firstrow) { skip_firstrow=false; return true; };
        values.emplace_back(std::string(keyp, keylen), value);
        if (values.size() == limit) {
            return false;
        }
        return true;
    }

    void set_limit(size_t llimit=10000) {limit=llimit;};
    void set_skip_firstrow(bool sskip_firstrow=false) {skip_firstrow=sskip_firstrow;};

    typedef std::pair<std::string, std::string> kv_pair;
    std::vector<kv_pair> values;
    bool skip_firstrow{false};
    size_t limit{INT_MAX};
};

void testDirect() {
    auto table_id = 20010;
    TThread::set_id(0);
    actual_directs::thread_init() ;  // init thread info for each thread
    new_directs database;
    int nums = 20 ;
    auto trd_cnt = 4 ;
    std::vector<std::thread> threads(trd_cnt);

    for (auto i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(concurrent_write_direct, i, nums, table_id, std::ref(database));
    }
    for (auto i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }

    // read keys from files
    vector<std::string> keys_insert ;
    for(int i=0; i < trd_cnt; i++) {
        std::string file_name = "./scripts/keys_" + std::to_string(i) ;
        std::ifstream file(file_name);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                keys_insert.emplace_back(line) ;
            }
            file.close();
        }
    }
    std::cout << "# of keys inserted: " << keys_insert.size() << std::endl;

    int notFound = 0 ;
    for(auto it: keys_insert) {
        if (!database[table_id].exists(it)) {
            notFound += 1 ;
            std::cout << "not found - recheck: " << it << "." << std::endl;
        }
    }
    std::cout << "# of keys inserted - no succeed: " << notFound << std::endl;

    // scan the Masstree
    mbta_wrapper *db = new mbta_wrapper;
    // mbta_ordered_index => MassTree, if it takes too long time
    mbta_ordered_index *user_table = db->open_index("user_0", table_id, std::ref(database[table_id])) ;
    const std::string startKey = "/";
    const std::string endKey = ":" ;
    new_scan_callback_bench calloc;
    str_arena arena;
    void *buf = NULL ;
    void *txn = db->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
    user_table->scan(txn, startKey, &endKey, calloc) ;
    std::cout << "# of scan: " << calloc.values.size() << std::endl;
}

void concurrent_write_trans(int tid, int nums, int table_id) {
    TThread::set_id(0);
    actual_directs::thread_init() ;  // init thread info for each thread
    std::string value = "111111" ;
    ofstream key_file("./scripts/keys_" + std::to_string(tid)) ;

    for (int j=0; j<nums; j++) {
        str_arena arena;
        void *buf = NULL ;
        void *txn = ndb->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
        unsigned int ui ;
        uint64_t commit_id = __rdtscp(&ui);
        std::string myKey = std::to_string(commit_id * 100 + tid) ;
        key_file << myKey ;
        key_file << "\n" ;
        user_table_global->put_mbta(txn, myKey, con_cmpFunc2, value);
        auto ret = ndb->commit_txn_no_paxos(txn);
        if (ret != 1)
            std::cout << "submitted key=" << myKey << ", status=" << ret << std::endl;
       // std::this_thread::sleep_for (std::chrono::milliseconds (10));
    }
    key_file.close() ;
}


void testTrans() {
    auto table_id = 20010;
    TThread::set_id(0);
    actual_directs::thread_init() ;  // init thread info for each thread
    new_directs database;

    int nums = 1000 ;
    auto trd_cnt = 10 ;
    std::vector<std::thread> threads(trd_cnt);

    for (auto i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(concurrent_write_trans, i, nums, table_id);
    }
    for (auto i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }

    // read keys from files
    vector<std::string> keys_insert ;
    for(auto i=0; i < trd_cnt; i++) {
        std::string file_name = "./scripts/keys_" + std::to_string(i) ;
        std::ifstream file(file_name);
        if (file.is_open()) {
            std::string line;
            while (std::getline(file, line)) {
                keys_insert.emplace_back(line) ;
            }
            file.close();
        }
    }
    std::cout << "# of keys inserted: " << keys_insert.size() << std::endl;

    // having concurrent updates
    for (auto i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(concurrent_write_trans, i, 2 * nums, table_id);
    }
    str_arena arena;
    void *buf = NULL ;

    // try to remove existing several keys
    /*
    for (auto i=0; i < 10; i++) {
        void *txnd = ndb->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
        user_table_global->remove(txnd, keys_insert[i]) ;
        auto ret = ndb->commit_txn(txnd);
        if (ret != 1)
            std::cout << "submitted key=" << keys_insert[i] << ", status=" << ret << std::endl;

    } */

    const std::string startKey = "/";
    const std::string endKey = ":" ;
    new_scan_callback_bench calloc;
    void *txn0 = ndb->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
    user_table_global->scan(txn0, startKey, &endKey, calloc) ;
    std::cout << "# of scan: " << calloc.values.size() << std::endl;

    // compare existing keys with scanned keys
    for (auto i: keys_insert) {
        bool valid = false;
        for (auto j: calloc.values) {
            if (i == j.first)
                valid = true ;
        }
        if (!valid) {
            std::cout << "key: " << i << " is lost\n" ;
        }
    }

    for (auto i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }
}

/**
 * expected output:
 *   new commit_id: 100, new value: 200, old commit_id: 1, old value: 2, update or not: 1
 *   new commit_id: 1000, new value: 2000, old commit_id: 100, old value: 200, update or not: 1
 *   new commit_id: 2, new value: 2, old commit_id: 1000, old value: 2000, update or not: 0
 *   new commit_id: 10, new value: 10, old commit_id: 1000, old value: 2000, update or not: 0
 */

void testCorrectness() {
    std::string key_str = "0000", value = "";
    value.resize(2 * sizeof(unsigned long long int));

    for (size_t i = 0; i <= 4; i++) {
        unsigned long long int commit_id;
        unsigned long long int trueValue;

        switch (i) {
            case 0:
                commit_id = 1;
                trueValue = 2;
                break;
            case 1:
                commit_id = 100;
                trueValue = 200;
                break;
            case 2:
                commit_id = 1000;
                trueValue = 2000;
                break;
            case 3:
                commit_id = 2;
                trueValue = 2;
                break;
            case 4:
                commit_id = 10;
                trueValue = 10;
                break;
        }

        con_keystore_encode3(value, commit_id, trueValue);

        str_arena arena;
        void *buf = NULL ;
        void *txn = ndb->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
        user_table_global->put_mbta(txn, key_str, con_cmpFunc2, value);
        auto ret = ndb->commit_txn_no_paxos(txn);
        if (ret != 1)
            std::cout << "update error" << std::endl;
    }
}

void transTransOne() {
    auto table_id = 20010;
    TThread::set_id(0);
    actual_directs::thread_init() ;  // init thread info for each thread
    new_directs database;

    concurrent_write_trans(0, 100, table_id) ;

    str_arena arena;
    void *buf = NULL ;
    const std::string startKey = "/";
    const std::string endKey = ":" ;
    new_scan_callback_bench calloc;
    void *txn0 = ndb->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
    user_table_global->scan(txn0, startKey, &endKey, calloc) ;
    std::cout << "# of scan: " << calloc.values.size() << std::endl;
}

void concurrent_write_trans_nolocal(int tid, int nums, int table_id) {
    TThread::set_id(0);
    actual_directs::thread_init() ;  // init thread info for each thread
    std::string value = "111111" ;

    for (int j=0; j<nums; j++) {
        str_arena arena;
        void *buf = NULL ;
        void *txn = ndb->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
        unsigned int ui ;
        uint64_t commit_id = __rdtscp(&ui);
        std::string myKey = std::to_string(commit_id * 100 + tid) ;
        //f_cnt.fetch_add(1, std::memory_order_relaxed);
        //std::string myKey = std::to_string(f_cnt) ;
        user_table_global->put_mbta(txn, myKey, con_cmpFunc2, value);
        auto ret = ndb->commit_txn_no_paxos(txn);
        if (ret != 1)
            std::cout << "submitted key=" << myKey << ", status=" << ret << std::endl;
    }
}

void scanBulk() {
    auto table_id = 20010;
    TThread::set_id(0);
    actual_directs::thread_init() ;  // init thread info for each thread
    new_directs database;

    int nums = 3111 ;
    auto trd_cnt = 10 ;
    std::cout << "start writing....\n" ;
    std::vector<std::thread> threads(trd_cnt);

    for (auto i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(concurrent_write_trans_nolocal, i, nums, table_id);
    }
    for (auto i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }

    std::cout << "completed writing...\n" ;
    str_arena arena;
    void *buf = NULL ;

//    std::string txn_obj_buf;
//    txn_obj_buf.reserve(str_arena::MinStrReserveLength);
//    txn_obj_buf.resize(ndb->sizeof_txn_object(1000));
//    void *bb = (void*)txn_obj_buf.data();
//    scoped_str_arena s_arena(arena);

    std::cout << "start scan....\n" ;
    std::string startKey = "/"; // code: 47 (0: 48)
    std::string endKey = ":" ;  // code: 58 (9: 57)
    std::string end_iter;
    bool startBulk = true;
    size_t bulkSize = 0;
    do {
        void *txn0 = ndb->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
        new_scan_callback_bench calloc;
        calloc.set_limit(10000);
        calloc.set_skip_firstrow(!startBulk);
        end_iter = user_table_global->scan_iter(txn0, startKey, &endKey, calloc) ;
        std::cout << "range (" << startKey << ", " << end_iter << "), len: " << calloc.values.size()  << std::endl;
        startKey = end_iter ;
        startBulk = false;
        bulkSize = calloc.values.size() ;
        ndb->commit_txn_no_paxos(txn0);
    } while (bulkSize) ;
}

int main()
{
    // testDirect() ; // jay's implementation, discard

    // attempt to insert one K-V pair by a single thread
    //transTransOne() ;

    // wrap every K-V pair as a transaction
    // 1. use multiple threads to insert K-V values; 2. check if all keys are there;
    //testTrans() ;

    // test on compare functionality
    //testCorrectness() ;

    // read a scan every 1 millions
    scanBulk() ;

    std::cout << "all tests DONE\n" ;
    return 0 ;
}