//
// Created by weihshen on 2/3/21.
//

#include <iostream>
#include <random>
#include <chrono>
#include <thread>
#include <algorithm>
#include "../benchmarks/bench.h"
#include "../benchmarks/mbta_wrapper.hh"

class new_scan_callback_bench : public abstract_ordered_index::scan_callback {
public:
    new_scan_callback_bench() {} ;
    virtual bool invoke(
            const char *keyp, size_t keylen,
            const string &value)
    {
        values.emplace_back(std::string(keyp, keylen), value);
        std::cout << "scan: " << std::string(keyp, keylen) << " -------> " << value << std::endl;
        return true;
    }

    typedef std::pair<std::string, std::string> kv_pair;
    std::vector<kv_pair> values;
};

class simple_tpcc_worker {
public:
    simple_tpcc_worker(abstract_db *db) : db(db) {
        txn_obj_buf.reserve(str_arena::MinStrReserveLength);
        txn_obj_buf.resize(db->sizeof_txn_object(0));
    }

    void txn_new_order() {
        static abstract_ordered_index *customerTable = simple_tpcc_worker::OpenTablesForTablespace(db, "customer_0") ;  // shared Masstree instance
        std::this_thread::sleep_for (std::chrono::seconds (1));

        // write
        for (size_t i=0; i < 5; i++) {
            void *txn = db->new_txn(0, arena, txn_buf(), abstract_db::HINT_TPCC_NEW_ORDER);
            std::string key = "prefix_" + std::to_string(i);
            std::string value = "" + std::to_string(rand() % 10);
            try {
                customerTable->put(txn, key, StringWrapper(value));
                auto ret = db->commit_txn(txn);
            } catch (abstract_db::abstract_abort_exception &ex) {
                std::cout << "abort key=" << key << std::endl;
                db->abort_txn(txn);
            }
        }

        // read
        for (size_t i=0; i < 5; i++) {
            void *txn = db->new_txn(0, arena, txn_buf(), abstract_db::HINT_TPCC_NEW_ORDER);
            std::string key = "prefix_" + std::to_string(i);
            std::string value = "";
            try {
                customerTable->get(txn, key, value);
                auto ret = db->commit_txn(txn);
                std::cout << "read value from key=" << key << ", value=" << value << std::endl;
            } catch (abstract_db::abstract_abort_exception &ex) {
                std::cout << "abort (read) key=" << key << std::endl;
                db->abort_txn(txn);
            }
        }

        // scan
        void *buf = NULL ;
        std::cout << "start scan....\n" ;
        char WS = static_cast<char>(0);
        std::string startKey(1, WS);
        char WE = static_cast<char>(255);
        std::string endKey(1, WE);
        new_scan_callback_bench calloc;
        void *txn0 = db->new_txn(0, arena, buf, abstract_db::HINT_DEFAULT);
        customerTable->scan(txn0, startKey, &endKey, calloc) ;
    }
    void run() {
        scoped_db_thread_ctx ctx(db, false);
    }
    static abstract_ordered_index * OpenTablesForTablespace(abstract_db *db, const char *name) {
       return db->open_index(name, 1, false, false); // create a table instance: mbta_ordered_index
    }
protected:
    abstract_db *const db;
    str_arena arena;
    std::string txn_obj_buf;
    inline void *txn_buf() { return (void *) txn_obj_buf.data(); }
};

void starter(abstract_db *db) {
    auto worker = new simple_tpcc_worker(db) ;
    worker->run() ;
    worker->txn_new_order() ;
}

int main() {
    register_sync_util([&]() {
        return 1;
    });

    abstract_db *db = new mbta_wrapper;

    auto trd_cnt = 1 ;
    std::vector<std::thread> threads(trd_cnt);

    for (auto i=0; i< trd_cnt; i++) {
        threads[i] = std::thread(starter, db);
    }
    for (auto i=0; i< trd_cnt; i++) {
        threads[i].join() ;
    }
    return 0;
}