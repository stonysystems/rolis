#!/bin/bash
repos="rolis-eurosys2022"  # repos name, default
workdir="~"  # we default put our repos under the root
leadrIP=$( cat ./scripts/ip_leader_replica )
p1=$( cat ./scripts/ip_p1_follower_replica )
p2=$( cat ./scripts/ip_p2_follower_replica )
ulimit -n 10000

repos="silo-sto"
workdir="~/weihai-projects"

# cleanup logs 
eval "cd $workdir/$repos/ && sudo rm -rf xxxx15/*"

batch_size=( 50 100 200 400 800 1600 3200 )
for i in "${batch_size[@]}"
do
   # run on the leader replica
   bash ./batch_silo.sh kill
   sleep 1
   echo "leader: cd $workdir/$repos/ && sudo bash ./b0.sh 16 $i"
   eval "cd $workdir/$repos/ && sudo bash ./b0.sh 16 $i"
   sleep 1

# run on the p1 follower replica
   echo "p1: ulimit -n 10000; cd $workdir/$repos/ && sudo bash ./b1.sh 16 $i"
   ssh $p1 "ulimit -n 10000; cd $workdir/$repos/ && sudo bash ./b1.sh 16 $i" &
   sleep 1

# run on the p2 follower replica
   echo "p2: ulimit -n 10000; cd $workdir/$repos/ && sudo bash ./b2.sh 16 $i"
   ssh $p2 "ulimit -n 10000; cd $workdir/$repos/ && sudo bash ./b2.sh 16 $i" &
   sleep 1

sleep 60
done
