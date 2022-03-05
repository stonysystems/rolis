# Skeleton code of SiLO-sto
> *updated - 2020-11-17*

dbtest.cc: https://github.com/mrityunjaykumar911/silo-sto/blob/no-table-differ/benchmarks/dbtest.cc
```
// 整个系统的入口函数
int  main(int argc, char **argv) {   
    // 1. 获取参数， 比如: bench, num-threads, scale-factor, paxos-config
    
    // 2. 启动Paxos with parameters - 1c1s1p.yaml, occ_paxos.yml
    setup(argc_paxos, argv_paxos);
    
    // 3. 做一些数据的准备工作
    db = new mbta_wrapper;
    
    for (int i=0; i<nthreads; i++) {
        register_for_follower(); 
        register_for_leader();  
    }
    
    // 4. 启动Paxos multi-threads
    setup2()

    
    // 5. 调用test_fn_with_runner = tpcc_do_test_run开始benchmarks
    bench_runner *R = test_fn_with_runner(db, argc, argv);

    // 6. 后续数据统计，比如用时
    
    // 7. 停止Paxos
    pre_shutdown_step(); 
    shutdown_paxos();
}
```

tpcc.cc: https://github.com/mrityunjaykumar911/silo-sto/blob/no-table-differ/benchmarks/tpcc.cc
```
bench_runner*  tpcc_do_test_run(abstract_db *db, int argc, char **argv) {
    // 1. 参数获取
    
    // 2. 调用run_without_stats 
    tpcc_bench_runner.run_without_stats()  
}

class tpcc_warehouse_loader {  // tpcc_order_loader
    protected void load() {
        // 1. tpcc数据导入: insert
        
        // 2. commit transaction，db(class mbta_wrapper)
        ALWAYS_ASSERT(db->commit_txn(txn));
    }
}

class tpcc_bench_runner {
    void make_workers() ;
    void make_loaders() ; 
}
```

bench.cc: https://github.com/mrityunjaykumar911/silo-sto/blob/no-table-differ/benchmarks/bench.cc
```
void bench_runner::run_without_stats() {   
    // 1. init tpcc loaders    
    make_loaders()

    // 2. init tpcc workers
    const vector<bench_worker *> workers = make_workers() ;
}
```

mbta_wrapper_norm.hh: https://github.com/mrityunjaykumar911/silo-sto/blob/no-table-differ/benchmarks/mbta_wrapper_norm.hh
```
class mbta_wrapper : public abstract_db {
    bool commit_txn(void *txn) { 
        return Sto::try_commit(); 
    }
}
```

Transaction.cc: https://github.com/mrityunjaykumar911/silo-sto/blob/no-table-differ/benchmarks/sto/Transaction.cc
```
bool Transaction::try_commit(bool no_paxos) { 
    // 1. serialize transasctions 
    serialize_util()
}

void Transaction::serialize_util() {
    // 分配内存, Transaction.hh => StringAllocator：
    //    1. local_strings.push (std::string((char *)array,w)) 
    //    2. (unsigned char *)malloc((6* tset_size_ * ul_len * sizeof(char))+1);
    
    // 1. 提交serialized transaction到paxos
    if (instance->checkPushRequired())  // do batch job
        add_log_to_nc();

    // 2. 释放内存
    free(array) ;
}

class StringAllocator {
    int kSizeLimit;  // batch size
}
```

paxos_main_helper.cc: https://github.com/shuaimu/janus/blob/ak-perf-multSiloOnePaxos/src/deptran/paxos_main_helper.cc
```
void add_log_to_nc() {
    add_log_without_queue((char*)log, len, par_id); 
}

void add_log_without_queue() {
    worker->Submit(log,len, par_id);
}

void register_for_follower() { }

void register_for_leader() { }
```

paxos_worker.cc: https://github.com/shuaimu/janus/blob/ak-perf-multSiloOnePaxos/src/deptran/paxos_worker.cc
```
void PaxosWorker::Submit(const char* log_entry, int length, uint32_t par_id) {
    _Submit(sp_m);
}

void PaxosWorker::_Submit() {
    coord = CreateCoordinator()
    
    // 1. 将coord推到PaxosWorker::coo_queue_nc里面
}

void PaxosWorker::StartReadAcceptNc() {
    // 1. 从queue中读取coord
    // 2. 调用BulkSubmit
}

void PaxosWorker::BulkSubmit() {
    coord.BulkSubmit() 
}

void PaxosWorker::Next() {
    // invoke callback function registered in dbtest.cc
    callback_(sp_log_entry.operation_test.get(), sp_log_entry.length) ;
}
```

coordinator.cc: https://github.com/shuaimu/janus/blob/ak-perf-multSiloOnePaxos/src/deptran/paxos/coordinator.cc
```
void BulkCoordinatorMultiPaxos::BulkSubmit() {
    GotoNextPhase();
}

void CoordinatorMultiPaxos::GotoNextPhase() {
   Accept()
   Commit()
}

void BulkCoordinatorMultiPaxos::Accept() { 
    // 通过RPC进行broadcast
    auto sp_quorum = commo()->BroadcastBulkAccept(par_id_, cmd_); 
}
```

commo.cc: https://github.com/shuaimu/janus/blob/ak-perf-multSiloOnePaxos/src/deptran/paxos/commo.cc
```
void MultiPaxosCommo::BroadcastAccept() {
    // 1. 遍历server proxy，然后发送RPC
    for (auto& p : proxies) { 
        auto proxy = (MultiPaxosProxy*) p.second; 
        // 2. 发送RPC
        auto f = proxy->async_Accept(slot_id, ballot, md, fuattr); 
        Future::safe_release(f); 
    }
}
```

s_main.cc: https://github.com/shuaimu/janus/blob/ak-perf-multSiloOnePaxos/src/deptran/s_main.cc
```
int main(int argc, char *argv[]) {
    // 可以用作测试Paxos的functions，可以查看如何调用Paxos
}

int setup(int argc, char* argv[]); 
int setup2(); 
int shutdown_paxos(); 
void microbench_paxos(); 
void register_for_follower(std::function<void(const char*, int)>, uint32_t); 
void register_for_leader(std::function<void(const char*, int)>, uint32_t); 
void submit(const char*, int, uint32_t);    // 最重要的commit一个log到Paxos
void add_log(const char*, int, uint32_t);
```

1c1s1p.yml: https://github.com/shuaimu/janus/blob/ak-perf-multSiloOnePaxos/config/1c1s1p.yml

occ_paxos.yml: https://github.com/shuaimu/janus/blob/ak-perf-multSiloOnePaxos/config/occ_paxos.yml

