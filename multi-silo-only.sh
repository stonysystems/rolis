#!/bin/bash
# ulimit -n 8000
# ----------------------------------------------------------------------------- compile ------------------------------------------------------------------------------------------
sudo pkill -f dbtest
mkdir -p silo-only-logs
make clean && make -j dbtest MODE=perf SERIALIZE=0 PAXOS_ENABLE_CONFIG=0 STO_BATCH_CONFIG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=0 OPTIMIZED_REPLAY=0 REPLAY_FOLLOWER=0 DBTEST_PROFILER=0

sleep 1

sstart=$1
eend=$2
defvalue=0
skip_cgroup=${3:-$defvalue}

# ----------------------------------------------------------------------------- RUN ------------------------------------------------------------------------------------------
for (( trd=$sstart; trd<=$eend; trd++ ))
do
  if ((skip_cgroup == 0)); then
    echo "starting CPU: $trd"
    sudo pkill -f dbtest
    sudo cgdelete -g cpuset:/cpulimitl
    sudo cgcreate -t $USER:$USER -a $USER:$USER  -g cpuset:/cpulimitl
    sudo cgset -r cpuset.mems=0 cpulimitl 
    sudo cgset -r cpuset.cpus=0-$trd cpulimitl
    sudo cgexec -g cpuset:cpulimitl ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc \
                                                                          --db-type mbta --scale-factor $trd --num-threads $trd \
                                                                          --numa-memory 1G --parallel-loading --runtime 30 \
                                                                          --bench-opts="--cpu-gap 1 --num-cpus 32" \
                                                                          -F third-party/paxos/config/local/1follower_$trd.yml \
                                                                          -F third-party/paxos/config/occ_paxos.yml \
                                                                          --paxos-leader-config --multi-process -P localhost -S 1000 > ./silo-only-logs/leader-$trd-1000.log 2>&1 &
  else
    ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc \
                                                                          --db-type mbta --scale-factor $trd --num-threads $trd \
                                                                          --numa-memory 1G --parallel-loading --runtime 30 \
                                                                          --bench-opts="--cpu-gap 1 --num-cpus 32" \
                                                                          -F third-party/paxos/config/local/1follower_$trd.yml \
                                                                          -F third-party/paxos/config/occ_paxos.yml \
                                                                          --paxos-leader-config --multi-process -P localhost -S 1000 > ./silo-only-logs/leader-$trd-1000.log 2>&1 &

  fi

    # sudo cgexec -g cpuset:cpulimitl ./out-perf.masstree/benchmarks/dbtest --verbose --bench micro --db-type mbta --scale-factor $trd --num-threads $trd --numa-memory 1G --parallel-loading --runtime 30 -F third-party/paxos/config/1silo_1paxos_1follower/$trd.yml -F third-party/paxos/config/occ_paxos.yml --paxos-leader-config --multi-process -P localhost -S 1000 > ./silo-only-logs/leader-$trd.log 2>&1 &
   let s=30+trd+10
   sleep $s
   echo "ending CPU: $trd"
done
