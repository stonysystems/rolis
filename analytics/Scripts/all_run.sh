#!/usr/bin/env bash
rm -rf ~/logs/*
rm -rf ~/prev_logs/*
echo "----------------------------------------"
echo "Removed logs from ~/logs and ~/prev_logs"
echo "----------------------------------------"

rm -f "/tmp/cpu$1$2.prof"
make clean
MODE=perf SERIALIZE=$1 CPU_PROFILING=$3 make -j dbtest
BENCH=./out-perf.masstree/benchmarks/dbtest

#CPUPROFILE=/tmp/cpu.prof $BENCH --verbose --bench tpcc --db-type mbta --scale-factor 10 --num-threads $2 --numa-memory 4G --parallel-loading --runtime 40
CPUPROFILE="/tmp/cpu$1$2.prof" $BENCH --verbose --bench tpcc --db-type mbta --scale-factor $2 --num-threads $2 --numa-memory 4G --parallel-loading --runtime 40
#$BENCH --verbose --bench tpcc --db-type mbta --scale-factor 10 --num-threads 10 --numa-memory 2G --parallel-loading --runtime 15
#$BENCH --verbose --bench tpcc --db-type mbta --scale-factor 12 --num-threads 15 --numa-memory 3G --parallel-loading --runtime 20
#$BENCH --verbose --bench tpcc --db-type mbta --scale-factor 12 --num-threads 15 --numa-memory 4G --parallel-loading --runtime 20
#$BENCH --verbose --bench tpcc --db-type mbta --scale-factor 20 --num-threads 20 --numa-memory 6G --parallel-loading --runtime 30
echo "----------------------------------------"
echo "Gen BenchMarks Done"
echo "----------------------------------------"
PWD="`pwd`"

#if [ "$1" = "1" ]; then
#	cd /home/jay/logs/
#	python /home/jay/src/cpp_proj/x/logger.csv . "/home/jay/csv_logs/$2/"
#	cd "$PWD"
#fi
#exit

cp -r ~/logs/* ~/prev_logs/
echo "----------------------------------------"
echo "<<<<<< copy done >>>>>>>>"
echo "----------------------------------------"

echo "----------------------------------------"
echo "running ht_mt2 @@@@@@"
make clean
MODE=perf make -j ht_mt2
echo "----------------------------------------"

limits=(3 5 8 10 12 15 20 1)
#limits=(1 3 5 8 10 12 15 20 30 40 50 63)
#limits=(5 8 10 12 15)
#limits=(1 3)

FILE_COUNTS="`python -c 'import os ; print(len(os.listdir("/home/jay/prev_logs/"))); '`"

for i in "${limits[@]}"; 
do
	echo "----------------------------------------"
	echo "processing for ${i} [start]" 
        echo "`./out-perf.masstree/benchmarks/ht_mt2 --file-count $FILE_COUNTS --num-threads ${i} > 1.txt`" 
	echo "processing for ${i} [stop] <<<< "
	echo "[LOGS OUTPUT] `python ~/src/pyp/read_fail_count.py 1.txt`"
	echo "processing for ${i} [stop] >>>> "
	echo "----------------------------------------"
done
