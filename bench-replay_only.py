import os.path
from os import path
from subprocess import call
import time
import datetime
import argparse
import os


log_commands, log_results = None, None

prefix = "/dev/shm/GLOGS/OPTIMAL"

"""
# automatic scripts
make clean && make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 STO_BATCH_CONFIG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=1 OPTIMIZED_REPLAY=1 REPLAY_FOLLOWER=0 DBTEST_PROFILER=0
make ht_mt2
sudo python3 bench-replay_only.py --output=x1 --skip_compile=1 --start_thread=1 --end_thread=31 --run=RUN --repeat=1

# for debugging manually
./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --bench-opts="--cpu-gap 1 --num-cpus 32" --scale-factor 3 --num-threads 3 --numa-memory 4G --parallel-loading --runtime 10 -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 1000
./out-perf.masstree/benchmarks/ht_mt2 --file-count 15 --file-path=/dev/shm/GLOGS/OPTIMAL/GenLogThd3.Time.10/ --num-threads 3 --optimized 1
"""
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
    20: 25,
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


COMMANDS = {
    # compiler: without Paxos involved
    "init-compiler": "make clean && make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=0 STO_BATCH_CONFIG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=1 OPTIMIZED_REPLAY=1 REPLAY_FOLLOWER=0 DBTEST_PROFILER=0 && make ht_mt2",
    # generate logs
    "generate": """./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --bench-opts="--cpu-gap 1 --num-cpus 32" --scale-factor {thread_num} --num-threads {thread_num} --numa-memory 4G --parallel-loading --runtime {duration} -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 1000 > {output}/{thread_num}-{trial}.G.log 2>&1 """,
    # replay logs
    "replay": """./out-perf.masstree/benchmarks/ht_mt2 --file-count {file_count} --file-path={prefix}/GenLogThd{thread_num}.Time.{duration}/ --num-threads {thread_num} --optimized 1 > {output}/{thread_num}-{trial}.R.log 2>&1""",
    "info": """cat {base_path}/info/Log-ThreadID-9888.txt {base_path}/info/Log-ThreadID-9999.txt && ls -lh {base_path}/ > {output}/{thread_num}-{trial}.info.log 2>&1"""
}


def logit(handler, text):
    handler.write("%s\n" % text)
    handler.flush()


def getInfo9888_V2(info9888):
    trans_cnt = {}
    if path.exists(info9888):
        with open(info9888, "r") as hanlder:
            for e in hanlder.readlines():
                print(e.strip())
                if "total_number," in e:
                    items = int(e.replace("total_number,", ""))
                    return items
    return 0


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
        print(trans_cnt, table_ids, total, file_count)
        return trans_cnt, table_ids, total, file_count
    else:
        return False, False, False, 0


# skipG: skip generation, skipR: skip running
def runner(start, end, it=3, skipG=False, skipR=False, output="results-replay", run=""):
    for thread_num in range(start, end+1):
        duration = settings[thread_num]
        for trial in range(1, it+1):
            basePath = "{prefix}/GenLogThd{thread_num}.Time.{duration}".format(thread_num=thread_num, duration=duration, prefix=prefix)

            if not skipG:
                call("rm -rf /dev/shm/*", shell=True)
                logit(log_commands, "rm -rf /dev/shm/*")
                gCommand = COMMANDS['generate'].format(thread_num=thread_num, duration=duration, trial=trial, output=output, prefix=prefix)
                logit(log_commands, gCommand)
                if run == "RUN":
                    call(gCommand, shell=True)
                    time.sleep(1)

            # check info 9888.txt to take out table information and # of transaction
            trans_cnt, table_ids, total, file_count = getInfo9888(basePath + "/info/Log-ThreadID-9888.txt")
            ttotal = getInfo9888_V2(basePath + "/info/Log-ThreadID-9888.txt")
            print("count file:", file_count)
            if run == "RUN" and not trans_cnt:
                print("[ERROR] info does not exist")
                continue

            info_command = COMMANDS['info'].format(thread_num=thread_num, base_path=basePath, duration=duration, trial=trial, output=output)
            logit(log_commands, info_command)
            if run == "RUN":
                call(info_command, shell=True)

            # start replaying logs
            if not skipR:
                replay_command = COMMANDS['replay'].format(file_count=file_count, thread_num=thread_num, duration=duration, trial=trial, output=output, prefix=prefix)
                logit(log_commands, replay_command)
                if run == "RUN":
                    call(replay_command, shell=True)

                    with open("{output}/{thread_num}-{trial}.R.log".format(output=output, thread_num=thread_num, trial=trial), "r") as handler:
                        for e in handler.readlines():
                            if "Time Taken" in e:
                                xx = float(e.replace("Time Taken : ", "").replace("millis", "").strip())
                                log_results.write("%s-%s\n%s\t%s\t%s\n" % (thread_num, trial, ttotal, xx/1000.0, round(ttotal / (xx / 1000.0), 2)))
                                log_results.flush()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""
        benchmark for replay-only
""")
    parser.add_argument('--output', default="results-replay", type=str, help='default: results-replay')

    parser.add_argument('--skip_compile', default=0, type=int, help='default: 0, options: 0 or 1')
    parser.add_argument('--skip_g', default=0, type=int, help='skip generation, default: 0')
    parser.add_argument('--skip_r', default=0, type=int, help='skip replay, default: 0')
    parser.add_argument('--repeat', default=3, type=int, help='default: 3')
    parser.add_argument('--start_thread', default=1, type=int, help='default: 1')
    parser.add_argument('--end_thread', default=31, type=int, help='default: 31')
    parser.add_argument('--run', default='', help='RUN, default: ""')

    args = parser.parse_args()

    if not os.path.exists(args.output):
        os.makedirs(args.output)

    log_commands, log_results = open(args.output + "/commands.log", "w"), open(args.output + "/results.log", "w")
    logit(log_commands, args)

    if args.skip_compile == 0:
        logit(log_commands, COMMANDS['init-compiler'])
        if args.run == "RUN":
            call(COMMANDS['init-compiler'], shell=True)

    for i in range(args.start_thread, args.end_thread+1):
        runner(i, i, args.repeat, args.skip_g, args.skip_r, args.output, args.run)

    log_results.close()
    log_commands.close()
