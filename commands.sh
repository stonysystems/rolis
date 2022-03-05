trd=$1
./batch_silo.sh kill
sleep 1
ssh 10.1.0.7 "ulimit -n 10000; cd ~/weihai-projects/silo-sto && ./f0.sh $trd" &
sleep 3
ssh 10.1.0.8 "ulimit -n 10000; cd ~/weihai-projects/silo-sto && ./f1.sh $trd" &
sleep 3
ssh 10.1.0.9 "ulimit -n 10000; cd ~/weihai-projects/silo-sto && ./f2.sh $trd" &
sleep 3
ssh 10.1.0.72 "ulimit -n 10000; cd ~/weihai-projects/silo-sto && ./f3.sh $trd" &
sleep 3
ssh 10.1.0.73 "ulimit -n 10000; cd ~/weihai-projects/silo-sto && ./f4.sh $trd" &

tail -f xxxx16/*
