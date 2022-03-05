#!/bin/bash
sudo cgdelete -g cpuset:/cpulimitl
mkdir -p xxxx15
sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitl
trd=$1
let yyml=trd+1
defvalue=1000
batch=${2:-$defvalue}
sudo cgset -r cpuset.mems=0 cpulimitl
sudo cgset -r cpuset.cpus=0-$trd cpulimitl
sudo cgexec -g cpuset:cpulimitl ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_2follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S $batch > ./xxxx15/leader-$trd-$batch.log 2>&1 &
#tail -f ./xxxx15/leader-$trd-$batch.log
