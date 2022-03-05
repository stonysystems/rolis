import os.path
from os import path
from subprocess import call
import time
import datetime



if __name__ == "__main__":
    #call("cd .. && git checkout LogToFile && make clean && MODE=perf SERIALIZE=0 PAXOS_ENABLE_CONFIG=0 REPLAY_FOLLOWER=0 STO_BATCH_CONFIG=0 DO_DISK_LOG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 make -j dbtest ", shell=True)

    wr = open("results.tmp.log", "w")
    for num_thread in range(1, 32):
        tmp = "tmp.9999.log"
        command = "cd .. && ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor %s --num-threads %s --numa-memory 1G --parallel-loading --runtime 30 -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 100 > %s 2>&1 \n" % (num_thread, num_thread, tmp)
        print(command)
        call(command, shell=True)
        with open("/home/AzureUser/projects/silo-sto/"+tmp, "r") as hanlder:
            for e in hanlder.readlines():
                # agg_persist_throughput: 980988 ops/sec
                if "agg_persist_throughput" in e:
                    print(e)
                    throughput = float(e.replace("agg_persist_throughput:", "").replace("ops/sec", "").strip())
                    wr.write("%s\t%s\n" % (num_thread, throughput))
                    wr.flush()
    wr.close()
