#!/bin/bash
# ulimit -n 8000
# ----------------------------------------------------------------------------- compile ------------------------------------------------------------------------------------------
sudo pkill -f dbtest
mkdir -p xxxx11
make clean && make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=1 STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=0 OPTIMIZED_REPLAY=1 REPLAY_FOLLOWER=1 DBTEST_PROFILER=0

sleep 1

trd=$1
trial=$2

# ----------------------------------------------------------------------------- RUN ------------------------------------------------------------------------------------------
# leader: 0 ~ $trd, follwer#1: $trd+1 ~ 2*$trd+1, follower#2: 2*$trd+2 ~ 3*$trd+2

let f1_bottom=trd+1
let f1_up=2*trd+1
sudo cgdelete -g cpuset:/cpulimitf
sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitf
sudo cgset -r cpuset.mems=0-1 cpulimitf 
sudo cgset -r cpuset.cpus=$f1_bottom-$f1_up cpulimitf
echo "./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts='--cpu-gap 1 --num-cpus 80' -F third-party/paxos/config/1silo_1paxos_2follower/$trd.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p1"
sudo cgexec -g cpuset:cpulimitf ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 80" -F third-party/paxos/config/1silo_1paxos_2follower/$trd.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p1  > ./xxxx11/follower-p1-$trd-$trial.log 2>&1 &

sleep 1

let f2_bottom=2*trd+2
let f2_up=3*trd+2
sudo cgdelete -g cpuset:/cpulimitf2
sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitf2
sudo cgset -r cpuset.mems=0-1 cpulimitf2 
sudo cgset -r cpuset.cpus=$f2_bottom-$f2_up cpulimitf2
echo "./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts='--cpu-gap 1 --num-cpus 80' -F third-party/paxos/config/1silo_1paxos_2follower/$trd.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2"
sudo cgexec -g cpuset:cpulimitf2 ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 80" -F third-party/paxos/config/1silo_1paxos_2follower/$trd.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2  > ./xxxx11/follower-p2-$trd-$trial.log 2>&1 &

sleep 1

sudo cgdelete -g cpuset:/cpulimitl
sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitl
sudo cgset -r cpuset.mems=0-1 cpulimitl 
sudo cgset -r cpuset.cpus=0-$trd cpulimitl
echo "./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts='--cpu-gap 1 --num-cpus 80' -F third-party/paxos/config/1silo_1paxos_2follower/$trd.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S 5000" 
sudo cgexec -g cpuset:cpulimitl ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 80" -F third-party/paxos/config/1silo_1paxos_2follower/$trd.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S 1000 > ./xxxx11/leader-$trd-$trial.log 2>&1 &

sleep 1

tail -f ./xxxx11/leader-$trd-$trial.log ./xxxx11/follower-p1-$trd-$trial.log ./xxxx11/follower-p2-$trd-$trial.log
