#make clean && make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=1 STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=0 OPTIMIZED_REPLAY=1 REPLAY_FOLLOWER=1 DBTEST_PROFILER=0
cd ..
SCRIPT=`realpath $0`
SCRIPTPATH=`dirname $SCRIPT`
sudo g++ -Imasstree -g -Wall  -MD -MP -Ithird-party/lz4 -Ithird-party/paxos/src -DCONFIG_H=\"config/config-perf.h\" -O0  -DREGULAR_LOG_SUBMIT -DLOG_FOLDER=\"/dev/shm/GLOGS/\" -include masstree/config.h -DREAD_MY_WRITES=0 -DHASHTABLE=0 -DUSE_JEMALLOC -c ./scripts/scan_benchmark.cpp -o out-perf.masstree/benchmarks/sto/scan_benchmark.o -std=c++17
sudo g++ -pthread -o out-perf.masstree/benchmarks/scan_benchmark out-perf.masstree/benchmarks/sto/scan_benchmark.o \
                out-perf.masstree/allocator.o out-perf.masstree/btree.o out-perf.masstree/core.o out-perf.masstree/counter.o \
                out-perf.masstree/memory.o out-perf.masstree/rcu.o out-perf.masstree/stats_server.o out-perf.masstree/thread.o \
                out-perf.masstree/ticker.o out-perf.masstree/tuple.o out-perf.masstree/txn_btree.o out-perf.masstree/txn.o out-perf.masstree/txn_proto2_impl.o \
                out-perf.masstree/varint.o out-perf.masstree/compiler.o out-perf.masstree/str.o out-perf.masstree/string.o out-perf.masstree/straccum.o out-perf.masstree/json.o \
                out-perf.masstree/kvthread.o out-perf.masstree/benchmarks/bench.o out-perf.masstree/benchmarks/encstress.o \
                out-perf.masstree/benchmarks/bid.o out-perf.masstree/benchmarks/queue.o out-perf.masstree/benchmarks/tpcc.o out-perf.masstree/benchmarks/tpcc_simple.o \
                out-perf.masstree/benchmarks/ycsb.o out-perf.masstree/benchmarks/sto/ThreadPool.o out-perf.masstree/benchmarks/sto/function_pool.o benchmarks/sto/OutputDataSerializer.cpp \
                benchmarks/sto/SerializeUtility.cpp out-perf.masstree/benchmarks/sto/ReplayDB.o out-perf.masstree/benchmarks/sto/Transaction.o \
                out-perf.masstree/benchmarks/sto/MassTrans.o out-perf.masstree/benchmarks/sto/TRcu.o \
                out-perf.masstree/benchmarks/sto/Packer.o third-party/paxos/build/libtxlog.so third-party/lz4/liblz4.so \
                -lpthread -lnuma -lrt -lprofiler -ljemalloc -lz -lrt -lcrypt -laio -ldl -lssl -lcrypto \
                -Lthird-party/lz4 -llz4 -Wl,-rpath,$SCRIPTPATH/third-party/lz4 -Lthird-party/paxos/build -DLOG_FOLDER=\"/dev/shm/GLOGS/\" -std=c++17
rm -rf ./scripts/keys_read
rm -rf ./scripts/keys_insert
sudo ./out-perf.masstree/benchmarks/scan_benchmark
