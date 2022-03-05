# ----------------------------------------------------------------------------- compile ------------------------------------------------------------------------------------------
sudo pkill -f dbtest
mkdir -p xxxx12

./out-perf.masstree/benchmarks/dbtest --verbose --bench micro --db-type mbta \
                                                                      --scale-factor 2 --num-threads 2 --numa-memory 1G \
                                                                      --parallel-loading --runtime 30 \
                                                                      -F third-party/paxos/config/local/1follower_3.yml \
                                                                      -F third-party/paxos/config/occ_paxos.yml \
                                                                      --paxos-leader-config --multi-process -P localhost -S 10000 > ./xxxx12/leader-2.log 2>&1 &

sleep 1

./out-perf.masstree/benchmarks/dbtest --verbose --bench micro --db-type mbta \
                                                                      --scale-factor 2 --num-threads 2 --numa-memory 1G \
                                                                      --parallel-loading --runtime 30 \
                                                                      -F third-party/paxos/config/local/1follower_3.yml \
                                                                      -F third-party/paxos/config/occ_paxos.yml \
                                                                      --multi-process -P p1  > ./xxxx12/follower-p1-2.log 2>&1 &
