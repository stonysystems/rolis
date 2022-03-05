import os.path
from os import path
from subprocess import call
import time
import datetime


res = open("results/results.log", "w")

settings = {
    1: 10,
    2: 10,
    3: 10,
    4: 10,
    5: 15,
    6: 15,
    7: 15,
    8: 15,
    9: 15,
    10: 15,
    11: 15,
    12: 20,
    13: 20,
    14: 20,
    15: 20,
    16: 20,
    17: 20,
    18: 20,
    19: 20,
    20: 35,
    21: 25,
    22: 25,
    23: 25,
    24: 25,
    25: 25,
    26: 30,
    27: 30,
    28: 30,
    29: 30,
    30: 30,
    31: 30,
    32: 30,
}

CMD = {
    "generate": """cd .. && \
                ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta \
                --scale-factor {thread_num} --num-threads {thread_num} --numa-memory 4G --parallel-loading --runtime {duration} \
                -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 100 > multi-replay/results/{thread_num}-{trial}.G.log
        """,
    "replay": """cd .. && \
            ./out-perf.masstree/benchmarks/ht_mt2 --file-count {file_count} --file-path=/dev/shm/GLOGS/GenLogThd{thread_num}.Time.{duration}/ --num-threads {thread_num} --control=profiler \
                > multi-replay/results/{thread_num}-{trial}.R.log
        """,
    "info": """cd .. && \
            cat {base_path}/info/Log-ThreadID:9888.txt {base_path}/info/Log-ThreadID:9999.txt \
                && ls -lh /dev/shm/GLOGS/GenLogThd{thread_num}.Time.{duration}/ > multi-replay/results/{thread_num}-{trial}.info.log
        """
    }


def getInfo9888(info9888):
    trans_cnt = {}
    if path.exists(info9888):
        with open(info9888, "r") as hanlder:
            for e in hanlder.readlines():
                print(e.strip())
                if "Log-ThreadID" in e:
                    items = e.replace("Log-ThreadID:", "").strip().split(",")
                    table_id, cnt = items[0], items[1]
                    trans_cnt[table_id] = int(cnt)

        table_ids = sorted([int(e) for e, _ in trans_cnt.items()])
        total = 0
        for _, cnt in trans_cnt.items():
            total += cnt

        file_count = table_ids[-1] + 1
        return trans_cnt, table_ids, total, file_count
    else:
        print("[ERROR] info does not exist")
        return False, False, False, False


def runner(start, end, it=3, skipG=False, skipR=False):
    for thread_num in range(start, end+1):
        duration = settings[thread_num]
        for trial in range(1, it+1):
            basePath = "/dev/shm/GLOGS/GenLogThd%d.Time.%s" % (thread_num, duration)
            wr = open("results/%d-%d.track.log" % (thread_num, trial), "w")

            if not skipG:
                call("cd .. && git checkout LogToFile", shell=True)
                call("rm -rf /dev/shm/*", shell=True)
                call("/usr/bin/python3 mem_monitor.py -o results/%d-%d.G.mem.log &" % (thread_num, trial), shell=True)
                call(CMD['generate'].format(thread_num=thread_num, duration=duration, trial=trial), shell=True)
                call("pkill -f mem_monitor.py", shell=True)

            time.sleep(2)

            # check info 9888.txt to take out table information and # of transaction
            trans_cnt, table_ids, total, file_count = getInfo9888(basePath + "/info/Log-ThreadID:9888.txt")
            if not trans_cnt:
                print("[ERROR] info does not exist")
                continue

            call(CMD['info'].format(thread_num=thread_num, base_path=basePath, duration=duration, trial=trial), shell=True)

            # start replaying logs
            if not skipR:
                call("cd .. && git checkout ntd-2jay", shell=True)

                call("/usr/bin/python3 mem_monitor.py -o results/%d-%d.R.mem.log &" % (thread_num, trial), shell=True)

                call(CMD['replay'].format(file_count=file_count, thread_num=thread_num, duration=duration, trial=trial), shell=True)
                call("pkill -f mem_monitor.py", shell=True)

                with open("results/{thread_num}-{trial}.R.log".format(thread_num=thread_num, trial=trial), "r") as handler:
                    for e in handler.readlines():
                        if "Time Taken" in e:
                            xx = float(e.replace("Time Taken : ", "").replace("millis", "").strip()) / 1000
                            wr.write("%s\t%s\t%s\n" % (total, xx, round(total / xx, 2)))
                            wr.flush()
                            res.write("%s-%s\n%s\t%s\t%s\n" % (thread_num, trial, total, xx, round(total / xx, 2)))
                            res.flush()

                call("mv ../ht_mt2.prof results/{thread_num}-{trial}.ht_mt2.prof".format(thread_num=thread_num, trial=trial), shell=True)
            wr.close()


if __name__ == "__main__":
    start, end = 1, 31 
    # call("./init_compiler.sh", shell=True)

    for i in range(start, end+1):
        runner(i, i, 3, False, False)
    res.close()
