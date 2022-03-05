gdb --args ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 2 --num-threads 2 --numa-memory 1G --parallel-loading --runtime 300 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/local/2follower_3.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p1

gdb --args ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 2 --num-threads 2 --numa-memory 1G --parallel-loading --runtime 300 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/local/2follower_3.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P p2

gdb --args ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 2 --num-threads 2 --numa-memory 1G --parallel-loading --runtime 300 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/local/2follower_3.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S 1000 -z 0 

gdb --args ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 2 --num-threads 2 --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/local/2follower_3.yml -F third-party/paxos/config/occ_paxos.yml --multi-process -P localhost -S 1000 -z 1   

sudo pkill -f dbtest

sudo kill $(ps aux | grep 'paxos-leader-config' | awk '{print $2}')