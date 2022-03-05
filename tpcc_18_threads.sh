#!/bin/bash
# ulimit -n 8000
# ----------------------------------------------------------------------------- compile ------------------------------------------------------------------------------------------
sudo pkill -f dbtest
ulimit -n 10000
mkdir -p xxxx12

./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta \
                                                                      --scale-factor 18 --num-threads 18 --numa-memory 1G \
                                                                      --parallel-loading --runtime 30 \
                                                                      --bench-opts="--cpu-gap 1 --num-cpus 32" \
                                                                      -F third-party/paxos/config/local/1follower_19.yml \
                                                                      -F third-party/paxos/config/occ_paxos.yml \
                                                                      --paxos-leader-config --multi-process -P localhost -S 1000 > ./xxxx12/leader-18.log 2>&1 &

sleep 1

./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta \
                                                                      --scale-factor 18 --num-threads 18 --numa-memory 1G \
                                                                      --parallel-loading --runtime 30 \
                                                                      --bench-opts="--cpu-gap 1 --num-cpus 32" \
                                                                      -F third-party/paxos/config/local/1follower_19.yml \
                                                                      -F third-party/paxos/config/occ_paxos.yml \
                                                                      --multi-process -P p1  > ./xxxx12/follower-p1-18.log 2>&1 &



sleep 1

tail -f xxxx12/*
