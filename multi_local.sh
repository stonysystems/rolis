#!/bin/bash
# ulimit -n 8000
# this script requires at least 8 cpu cores locally
# ----------------------------------------------------------------------------- compile ------------------------------------------------------------------------------------------
sudo pkill -f dbtest
mkdir -p xxxx12
trd=$1
let yyml=trd+1
defvalue=1
skip=${2:-$defvalue}
if ((skip == 0)); then
 ./multi.sh
fi

sleep 1

# ----------------------------------------------------------------------------- RUN ------------------------------------------------------------------------------------------
#let cpucap=trd+4
#sudo cgdelete -g cpuset:/cpulimitf
#sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitf
#sudo cgset -r cpuset.mems=0 cpulimitf
#sudo cgset -r cpuset.cpus=4-$cpucap cpulimitf
#sudo cgexec -g cpuset:cpulimitf ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta \
./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta \
                                                                      --scale-factor $trd --num-threads $trd --numa-memory 1G \
                                                                      --parallel-loading --runtime 10 \
                                                                      --bench-opts="--cpu-gap 1 --num-cpus 32" \
                                                                      -F third-party/paxos/config/local/1follower_$yyml.yml \
                                                                      -F third-party/paxos/config/occ_paxos.yml \
                                                                      --multi-process -P p1  > ./xxxx12/follower-p1-$trd.log 2>&1 &

sleep 1

#sudo cgdelete -g cpuset:/cpulimitl
#sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitl
#sudo cgset -r cpuset.mems=0 cpulimitl
#sudo cgset -r cpuset.cpus=0-$trd cpulimitl
#sudo cgexec -g cpuset:cpulimitl ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta \
./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta \
                                                                      --scale-factor $trd --num-threads $trd --numa-memory 1G \
                                                                      --parallel-loading --runtime 10 \
                                                                      --bench-opts="--cpu-gap 1 --num-cpus 32" \
                                                                      -F third-party/paxos/config/local/1follower_$yyml.yml \
                                                                      -F third-party/paxos/config/occ_paxos.yml \
                                                                      --paxos-leader-config --multi-process -P localhost -S 1000 > ./xxxx12/leader-$trd.log 2>&1 &

#sudo cgexec -g cpuset:cpulimitf ./out-perf.masstree/benchmarks/dbtest --verbose --bench micro --db-type mbta \
#                                                                      --scale-factor $trd --num-threads $trd --numa-memory 1G \
#                                                                      --parallel-loading --runtime 30 \
#                                                                      -F third-party/paxos/config/local/1follower_$yyml.yml \
#                                                                      -F third-party/paxos/config/occ_paxos.yml \
#                                                                      --paxos-leader-config --multi-process -P localhost -S 1000

taildefault=1
istail=${3:-$taildefault}
if ((istail == 1)); then
  tail -f ./xxxx12/leader-$trd.log ./xxxx12/follower-p1-$trd.log
fi
