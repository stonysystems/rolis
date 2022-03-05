repos="rolis-eurosys2022"  # repos name, default
workdir="~"  # we default put our repos under the root
leadrIP=$( cat ./scripts/ip_leader_replica )
p1=$( cat ./scripts/ip_p1_follower_replica )
p2=$( cat ./scripts/ip_p2_follower_replica )
clients=(
      $p1 # p1 follower replica IP
      $p2 # p2 follower replica IP
      "10.1.0.74" # it is only required for networked clients
)

repos="silo-sto"
workdir="~/weihai-projects"

cmd1=""
cmd2="sudo skill dbtest;sudo pkill dbtest; sudo pkill nc_main; sleep 1"
cmd3="sudo rm -rf $workdir/$repos/xxxx15/*; sudo rm -rf $workdir/$repos/xxxx15_micro/*" # sudo rm -rf ~/meerkat/logs/* && sudo rm -rf /tmp/* && sudo rm -rf ~/boost_1_70_0 ~/boost_1_70_0.tar.bz2 ~/dpdk-19.11.5.tar.gz ~/dpdk-stable-19.11.5 ~/eRPC"
cmd4=""
cmd5=""
cmd6="scp -r $USER@$p1:$workdir/$repos/$2 tmp-000.log; scp -r $USER@$p2:$workdir/$repos/$2 tmp-111.log; cat tmp-000.log tmp-111.log > p1p2.log; rm tmp-000.log; rm tmp-111.log"

# for the leader replica
if [ $1 == 'scp' ]; then
	:
elif [ $1 == 'kill' ]; then
    echo "kill local"
    eval	$cmd2
elif [ $1 == 'del' ]; then
    echo "del local"
    eval	$cmd3
elif [ $1 == 'install' ]; then 
	:
elif [ $1 == 'init' ]; then
	:
elif [ $1 == 'copy_remote_file' ]; then
    echo "copy the remote file, cmd: $cmd6"
    eval $cmd6
else
  :
fi


cmd1="cd $workdir ; sudo rm -rf $repos; scp -r $username@$leadrIP:$workdir/$repos ."
cmd2="sudo skill dbtest;sudo pkill dbtest; sudo pkill nc_main; sleep 1"
cmd3="sudo rm -rf $workdir/$repos/xxxx15/*; sudo rm -rf $workdir/$repos/xxxx15_micro/*"
cmd4=""
cmd5=""
cmd6=""

# for the two follower replicas
for host in ${clients[@]}
do
  if [ $1 == 'scp' ]; then
    echo "scp to $host cmd: $cmd1 "
    ssh $host "$cmd1" &
  elif [ $1 == 'kill' ]; then
    echo "kill host $host"
    ssh $host "$cmd2" &
  elif [ $1 == 'del' ]; then
    echo "kill host $host"
    ssh $host "$cmd3" &
  elif [ $1 == 'install' ]; then 
	  :
  elif [ $1 == 'init' ]; then
	  :
  else
	  :
  fi
done

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


