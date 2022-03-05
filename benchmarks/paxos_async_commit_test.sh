cd ..

# Paxos build
#cd third-party/paxos
#rm -rf build
#./waf configure -p build

make paxos_async_commit_test
pkill -f paxos_async_commit_test

./out-perf.masstree/benchmarks/paxos_async_commit_test -n 100 -F third-party/paxos/config/1silo_1paxos_2follower/3.yml  -F third-party/paxos/config/occ_paxos.yml --multi-process -P p1 --num-of-partitions 3 > follower-p1.log 2>&1 &
./out-perf.masstree/benchmarks/paxos_async_commit_test -n 100 -F third-party/paxos/config/1silo_1paxos_2follower/3.yml  -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2 --num-of-partitions 3 > follower-p2.log 2>&1 &
./out-perf.masstree/benchmarks/paxos_async_commit_test -n 100 -F third-party/paxos/config/1silo_1paxos_2follower/3.yml  -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost --num-of-partitions 3 -t 10 > leader.log 2>&1 &

tail -f leader.log follower-p1.log follower-p2.log