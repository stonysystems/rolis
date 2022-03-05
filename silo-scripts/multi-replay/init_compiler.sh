cd ../
#rm -rf /dev/shm/*
rm ht_mt2.prof
git checkout LogToFile
make clean
make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 REPLAY_FOLLOWER=0

git checkout ntd-2jay
make ht_mt2
