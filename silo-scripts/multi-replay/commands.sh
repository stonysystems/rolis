#cd third-party/paxos
#rm -rf build
#./waf configure -p build
#cd ../..

#rm -rf ./out-perf.masstree
#make clean

# make ht_mt2
# make paxos_async_commit_test

# serialize + Paxos
# compiledb make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=1 REPLAY_FOLLOWER=0 STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0

# ONLY serialize
# compiledb make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 REPLAY_FOLLOWER=0 STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0

# google-pprof --pdf ./out-perf.masstree/benchmarks/dbtest abc.prof > tmp.pdf

# nautilus ./

# replay
# make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 REPLAY_FOLLOWER=0

#./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 4 --num-threads 4 --numa-memory 4G --parallel-loading --runtime 10 -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 1000

#echo "####### start working on threads of $1"
#rm -rf /dev/shm/* 
#git checkout LogToFile
#rm -rf ./out-perf.masstree
#sh log_collect.sh $1 $1 10
# counter 
#echo "###########################################"
#cat /dev/shm/GLOGS/GenLogThd$1.Time.10/info/Log-ThreadID\:9888.txt
#ls -lh /dev/shm/GLOGS/GenLogThd$1.Time.10/

# replay
#git checkout weihai-multi
#make clean
#rm -rf ./out-perf.masstree
#make ht_mt2
#cnt=$(find /dev/shm/GLOGS/GenLogThd$1.Time.10/ -maxdepth 1 -type f|wc -l)
#./out-perf.masstree/benchmarks/ht_mt2 --file-count ${cnt} --file-path="/dev/shm/GLOGS/GenLogThd$1.Time.10/" --num-threads $1 

#./out-perf.masstree/benchmarks/ht_mt2 --file-count 83 --file-path=/dev/shm/GLOGS/GenLogThd20.Time.35/ --num-threads 20 --log-size 49152

#./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 20 --num-threads 20 --numa-memory 4G --parallel-loading --runtime 35 -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 100

#limits=(100663296 50331648 25165824 12582912 6291456 3145728 1572864 786432 393216 196608 98304 49152 24576 12288 4800 2400)
limits=(56623104 28311552 14155776 7077888 3538944 1769472 884736 442368 221184 110592 55296 27648 13824 6912 2700 1350)
for i in "${limits[@]}"
do
   echo "############################################"
   ./out-perf.masstree/benchmarks/ht_mt2 --file-count 83 --file-path=/dev/shm/GLOGS/GenLogThd20.Time.35/ --num-threads 20 --log-size $i
   #mv abc ./tmp-2/abc-$i.prof
   #google-pprof --pdf ./out-perf.masstree/benchmarks/ht_mt2 ./tmp-2/abc-$i.prof > ./tmp-3/ht_mt2-$i.pdf
done
#./out-perf.masstree/benchmarks/ht_mt2 --file-count 83 --file-path=/dev/shm/GLOGS/GenLogThd20.Time.35/ --num-threads 20 --log-size 27648 
#./out-perf.masstree/benchmarks/ht_mt2 --file-count 83 --file-path=/dev/shm/GLOGS/GenLogThd20.Time.35/ --num-threads 20 --log-size 56623104 

# SiLO ONLY
MODE=perf SERIALIZE=0 PAXOS_ENABLE_CONFIG=0 REPLAY_FOLLOWER=0 STO_BATCH_CONFIG=0 DO_DISK_LOG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 make -j dbtest
