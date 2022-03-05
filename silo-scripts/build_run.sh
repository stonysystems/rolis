# build paxos
cd third-party/paxos
rm -rf build
./waf configure -p build
# run
./build/deptran_server -f config/janus.yml -f config/1c1s1p.yml -f config/tpca.yml -P localhost -d 20


# build ht_mt2
git checkout weihai-multi
rm -rf ./out-perf.masstree
make clean
make ht_mt2
# run ht_mt2
./out-perf.masstree/benchmarks/ht_mt2 --file-count 15 --file-path /dev/shm/GLOGS/GenLogThd1.Time.10/ --num-threads 1


# build paxos_test
rm -rf ./out-perf.masstree
make clean
make paxos_async_commit_test
# run paxos_async_commit_test
./out-perf.masstree/benchmarks/paxos_async_commit_test -n 1


# serialize + Paxos
compiledb make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=1 REPLAY_FOLLOWER=0 STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0

# ONLY serialize
compiledb make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 REPLAY_FOLLOWER=0 STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0


# replay
# build
compiledb make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 REPLAY_FOLLOWER=0
# run
./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 21 --num-threads 21 --numa-memory 4G --parallel-loading --runtime 10 -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 1000


# google-pprof to pdf
google-pprof --pdf ./out-perf.masstree/benchmarks/dbtest abc.prof > abc.pdf


# scp files from AzureUser
scp -r weihaishen@resto.eastus.cloudapp.azure.com:/home/AzureUser/m2.log .