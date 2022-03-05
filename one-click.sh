#!/bin/bash
repos="rolis-eurosys2022"  # repos name, default
workdir="~"  # we default put our repos under the root
leadrIP=$( cat ./scripts/ip_leader_replica )
p1=$( cat ./scripts/ip_p1_follower_replica )
p2=$( cat ./scripts/ip_p2_follower_replica )
ulimit -n 10000
# minimum of the number of worker threads
start=1
# maximum of the number of worker threads
end=31

repos="silo-sto"
workdir="~/weihai-projects"

setup () {
    bash ./batch_silo.sh kill
    mkdir -p results
    #rm ./results/*
}

experiment1 () {
   echo 'start experiment-1 - silo-only tpcc'
   sudo ./multi-silo-only.sh $start $end
   python3 scripts/extractor.py 0 silo-only-logs "agg_throughput:" "ops/sec" > results/silo-only-tpcc.log
}

experiment2 () {
    echo 'start experiment-2 - scalability-tpcc'
    sudo bash ./multi.sh
    bash ./batch_silo.sh kill
    bash ./batch_silo.sh scp

    eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b.py $start $end 1" &
    sleep 1

    ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2.py $start $end 1" &
    sleep 1

    ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1.py $start $end 1" &
    sleep 1

    echo "Wait for jobs..."
    FAIL=0

    for job in `jobs -p`
    do
        wait $job || let "FAIL+=1"
    done

    if [ "$FAIL" == "0" ];
    then
        echo "YAY!"
    else
        echo "FAIL! ($FAIL)"
    fi

    python3 scripts/extractor.py 0 xxxx15 "agg_throughput:" "ops/sec" > results/scalability-tpcc.log
}

experiment3 () {
  echo 'start experiment-3 - silo-only ycsb'
  sudo ./multi-silo-only-m.sh $start $end
  python3 scripts/extractor.py 0 silo-only-logs-m "agg_throughput:" "ops/sec" > results/silo-only-ycsb.log
}


experiment4 () {
    echo 'start experiment-4: scalability-ycsb'
    sudo bash ./multi.sh
    bash ./batch_silo.sh kill
    bash ./batch_silo.sh scp

    eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b_m.py $start $end 1" &
    sleep 1

    ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2_m.py $start $end 1" &
    sleep 1

    ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1_m.py $start $end 1" &
    sleep 1

    echo "Wait for jobs..."
    FAIL=0

    for job in `jobs -p`
    do
        wait $job || let "FAIL+=1"
    done

    if [ "$FAIL" == "0" ];
    then
        echo "YAY!"
    else
        echo "FAIL! ($FAIL)"
    fi

    python3 scripts/extractor.py 0 xxxx15_micro  "agg_throughput:" "ops/sec" > results/scalability-ycsb.log
}

experiment5() {
  echo "skip experiment 5"
}

experiment6() {
  echo 'start experiment-6: fail-over experiment'
  sudo  bash ./multi-failover.sh
  bash ./batch_silo.sh kill
  bash ./batch_silo.sh scp

  sudo  bash ./multi-failover-variable.sh
  
  eval "ulimit -n 10000; cd $workdir/$repos/ && sudo ./b0.sh $1" &
  sleep 1

  ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && sudo ./b2.sh $1" &
  sleep 1

  ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && sudo ./b1.sh $1 " &
  sleep 1

  echo "Wait for jobs..."
  FAIL=0

  for job in `jobs -p`
  do
      wait $job || let "FAIL+=1"
  done

  if [ "$FAIL" == "0" ];
  then
      echo "YAY!"
  else
      echo "FAIL! ($FAIL)"
  fi


  # 30 + 16 + 10 + 30
  sleep 86
  
  ./batch_silo.sh copy_remote_file ./xxxx15/follower-$1.log  && mv p1p2.log ./scripts/failure_follower && cp ./xxxx15/leader-$1-1000.log ./scripts/failure_leader

  python ./scripts/failure_cal.py > ./results/failover-$1-throughput.log
}

experiment7 () {
  echo "start experiment7 - latency of different batches"
  sudo  bash ./multi-latency.sh
  bash ./batch_silo.sh scp
  bash ./batch_silo.sh kill
  bash ./batch_size_exp.sh
  ag '% latency' xxxx15 > results/batch-latency.log
}

experiment8 () {
  echo "start experiment8 - throughput of different batches"
  sudo  bash ./multi.sh
  bash ./batch_silo.sh scp
  bash ./batch_silo.sh kill
  bash ./batch_size_exp.sh
  ag 'agg_throughput: ' xxxx15 > results/batch-throughput.log
}

experiment9 () {
    echo 'start experiment-9: single paxos stream'
    sudo bash ./multi_single_paxos.sh
    bash ./batch_silo.sh kill
    bash ./batch_silo.sh scp

    eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b.py $start $end 1" &
    sleep 1

    ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2.py $start $end 1" &
    sleep 1

    ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1.py $start $end 1" &
    sleep 1

    echo "Wait for jobs..."
    FAIL=0

    for job in `jobs -p`
    do
        wait $job || let "FAIL+=1"
    done

    if [ "$FAIL" == "0" ];
    then
        echo "YAY!"
    else
        echo "FAIL! ($FAIL)"
    fi

    python3 scripts/extractor.py 0 xxxx15 "agg_throughput:" "ops/sec" > results/single_paxos_stream.log
}

experiment10 () {
   echo "start experiment-10: skewed workload - Rolis"
   sudo bash ./multi-skewed.sh
   bash ./batch_silo.sh kill
   bash ./batch_silo.sh scp

   eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b_skew.py $start $end 1" &
   sleep 1

   ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2_skew.py $start $end 1" &
   sleep 1

   ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1_skew.py $start $end 1" &
   sleep 1

   echo "Wait for jobs..."
   FAIL=0

   for job in `jobs -p`
   do
       wait $job || let "FAIL+=1"
   done

   if [ "$FAIL" == "0" ];
   then
       echo "YAY!"
   else
       echo "FAIL! ($FAIL)"
   fi

   python3 scripts/extractor.py 0 xxxx15 "agg_throughput:" "ops/sec" > results/throughput_skewed_workload-rolis.log
}

experiment11() {
   echo "start experiment-11: skewed workload - Silo"
   sudo bash ./multi-silo-skewed.sh $start $end
   python3 scripts/extractor.py 0 silo-only-logs-skewed "agg_throughput:" "ops/sec" > results/silo-only-skewed.log
}

experiment12 () {
  echo "start experiment-12: replay only"
  mkdir -p ./replay-only
  sudo make clean
  make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 STO_BATCH_CONFIG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=1 OPTIMIZED_REPLAY=1 REPLAY_FOLLOWER=0 DBTEST_PROFILER=0
  make ht_mt2
  sudo python3 bench-replay_only.py --output="./replay-only" --skip_compile=1 --start_thread=$start --end_thread=$end --run=RUN --repeat=3
  python scripts/replay_only_cal.py replay-only/results.log  > results/replay-only.log
}


experiment13_1 () {
   echo "start experiment-13: breakdown of the system - Silo-only"
   eval "sudo timedatectl set-ntp true"
   ssh $p2 "sudo timedatectl set-ntp true"
   ssh $p1 "sudo timedatectl set-ntp true"

   eval "python3 mem_monitor.py -o 'ds0.log'" & 
   sudo ./multi-silo-only.sh 16 16 
   eval "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   python3 mem_analysis.py "./silo-only-logs/leader-16-1000.log" "ds0.log" "" ""  > tmp-experiment13_1.log
}

experiment13_2 () {
   echo "start experiment-13: breakdown of the system - Silo-serialization"
   sudo bash ./multi_no_add_log_to_nc.sh
   bash ./batch_silo.sh kill
   bash ./batch_silo.sh scp
   sleep 10

   eval "python3 mem_monitor.py -o 'ds0.log'" & 
   eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b.py 16 16 1" &
   sleep 1

   ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2.py 16 16 1" &
   sleep 1

   ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1.py 16 16 1" &
   sleep 1

   # 16 + 30 + 30
   sleep 76
   eval "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   python3 mem_analysis.py "./xxxx15/leader-16-1000.log" "ds0.log" "" ""  > tmp-experiment13_2.log
}

experiment13_3 () {
   echo "start experiment-13: breakdown of the system - Silo-serialization+replication"
   sudo bash ./multi_disable_replay.sh
   bash ./batch_silo.sh kill
   bash ./batch_silo.sh scp
   rm ds0.log ds1.log ds2.log

   eval "python3 mem_monitor.py -o 'ds0.log'" & 
   eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b.py 16 16 1" &
   sleep 1

   ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 mem_monitor.py -o 'ds2.log'" &
   ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2.py 16 16 1" &
   sleep 1

   ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 mem_monitor.py -o 'ds1.log'" &
   ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1.py 16 16 1" &
   sleep 1

   sleep 86

   eval "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   ssh $p2 "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   ssh $p1 "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   scp -r azureuser@$p1:$workdir/$repos/ds1.log .
   scp -r azureuser@$p2:$workdir/$repos/ds2.log .
   python3 mem_analysis.py "./xxxx15/leader-16-1000.log" "ds0.log" "ds1.log" "ds2.log"  > tmp-experiment13_3.log
}

experiment13_4 () {
   echo "start experiment-13: breakdown of the system - Silo-serialization+replication+replay"
   rm ds0.log ds1.log ds2.log
   sudo ./multi.sh
   bash ./batch_silo.sh kill
   bash ./batch_silo.sh scp

   eval "python3 mem_monitor.py -o 'ds0.log'" & 
   eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b.py 16 16 1" &
   sleep 1

   ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 mem_monitor.py -o 'ds2.log'" &
   ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2.py 16 16 1" &
   sleep 1

   ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 mem_monitor.py -o 'ds1.log'" &
   ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1.py 16 16 1" &
   sleep 1

   sleep 86 

   eval "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   ssh $p2 "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   ssh $p1 "ps aux|grep 'mem_monitor'| awk '{print \$2}' | xargs kill -9 \$1"
   scp -r azureuser@$p1:$workdir/$repos/ds1.log .
   scp -r azureuser@$p2:$workdir/$repos/ds2.log .
   python3 mem_analysis.py "./xxxx15/leader-16-1000.log" "ds0.log" "ds1.log" "ds2.log"  > tmp-experiment13_4.log
}

experiment13_collect() {
   cat tmp-experiment13_1.log tmp-experiment13_2.log tmp-experiment13_3.log tmp-experiment13_4.log > results/breakdown.log
}


experiment14_s() {
    # a machine with 32-core as the client is required, here I use 10.1.0.74
    eval "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/leader_b.py $1 $1 1" &
    sleep 1

    ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b2.py $1 $1 1" &
    sleep 1

    ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && python3 scripts/follower_b1.py $1 $1 1" &
    sleep 1

    ssh "10.1.0.74" "ulimit -n 10000; cd $workdir/$repos/ && ./third-party/paxos/build/nc_main 0 $1 '$leadrIP'" &

    sleep 90
    ssh "10.1.0.74" "sudo pkill -f nc_main"
    ./batch_silo.sh kill
}

experiment14() {
   echo "experiment14: networked clients\n"
   make paxos
   sudo ./multi_client.sh
   ./batch_silo.sh kill
   ./batch_silo.sh scp

   for (( trd=$start; trd<=$end; trd++ ))
do
   ./batch_silo.sh kill
   experiment14_s $trd
done
}

experiment14_local() {
   echo "testing experiment14: networked clients"
   sudo pkill -f dbtest
   sudo pkill -f nc_main
   # 1. compile
   make paxos
   sudo ./multi_client.sh
   
   # 2. start Rolis with 2 threads
   ./tpcc_2_threads.sh &

   # 3. start networked clients
   ./third-party/paxos/build/nc_main 0 2 "127.0.0.1" &

   sleep 60
   sudo pkill -f dbtest
   sudo pkill -f nc_main
}

experiment14_ycsb_local() {
   echo "testing experiment14: networked clients -ycsb"
   sudo pkill -f dbtest
   sudo pkill -f nc_main
   # 1. compile
   make paxos
   sudo ./multi_client_ycsb.sh
   
   # 2. start Rolis with 2 threads
   ./ycsb_2_threads.sh &

   # 3. start networked clients
   ./third-party/paxos/build/nc_main 0 2 "127.0.0.1" &

   sleep 60
   sudo pkill -f dbtest
   sudo pkill -f nc_main
}

#setup
## reproduce results reported in the paper
#experiment1
#experiment2
#experiment3
#experiment4
#experiment5
#experiment6 4
#experiment6 8
#experiment6 16
#experiment7
#experiment8
#experiment9
#experiment10
#experiment11
#experiment12
#experiment13_1
#experiment13_2
#experiment13_3
#experiment13_4
#experiment13_collect

# for testing experiments
#experiment14
#experiment14_local
experiment14_ycsb_local
