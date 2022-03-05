#!/bin/bash
sudo cgdelete -g cpuset:/cpulimitl
mkdir -p xxxx14
sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitl
trd=$1
let yyml=trd+1
sudo cgset -r cpuset.mems=0 cpulimitl 
sudo cgset -r cpuset.cpus=0-$trd cpulimitl
sudo cgexec -g cpuset:cpulimitl ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1silo_1paxos_1follower/$yyml.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S 1000 > ./xxxx14/leader-$trd.log 2>&1 &
#tail -f ./xxxx14/leader-$trd.log
