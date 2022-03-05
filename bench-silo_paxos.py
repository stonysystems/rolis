import os.path
from os import path
from subprocess import call
import time
import datetime
import argparse
import os


bytes27 = True 

COMMANDS = {
    # compile, USE_MALLOC_MODE=2 => tcmalloc
    "silo-only":  "make clean && make -j dbtest MODE=perf SERIALIZE=0 PAXOS_ENABLE_CONFIG=0 STO_BATCH_CONFIG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=0 OPTIMIZED_REPLAY=" + ("0" if bytes27 else "1"),
    "silo-paxos": "make clean && make -j dbtest MODE=perf SERIALIZE=1 PAXOS_ENABLE_CONFIG=1 STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=0 OPTIMIZED_REPLAY=" + ("0" if bytes27 else "1")
}

def logit(handler, text):
    handler.write("%s\n" % text)
    handler.flush()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""
	benchmark for SiLo-Paxos - don\'t run it in parallel
        e.g., python3 silo-paxos-bench.py --enable_paxos=1 --enable_replay=0 --bulk_size=100 --replica_num=1 --start_thread=3 --end_thread=4 --run=RUN --output=results
""")
    parser.add_argument('--enable_paxos', default=0, type=int, help='default: 0, enable SiLO-Paxos, options: 0 or 1')
    parser.add_argument('--enable_replay', default=0, type=int, help='default: 0, enable replaying Paxos callback, options: 0 or 1')
    parser.add_argument('--bulk_size', default=100, type=int, help='default: 100')
    parser.add_argument('--replica_num', default=1, type=int, help='default: 1, ONLY support 1 or 2')
    parser.add_argument('--output', default="results-log", help='default: results-log')

    parser.add_argument('--skip_compile', default=0, type=int, help='default: 0, options: 0 or 1')
    parser.add_argument('--run_time', default=30, type=int, help='default: 30')
    parser.add_argument('--start_thread', default=1, type=int, help='default: 1')
    parser.add_argument('--end_thread', default=31, type=int, help='default: 31')
    parser.add_argument('--enable_profiler', default=0, type=int, help='default: 0')
    parser.add_argument('--force_cpu', default=0, type=int, help='default: 0')
    parser.add_argument('--run', default='', help='RUN')

    args = parser.parse_args()

    if not os.path.exists(args.output):
         os.makedirs(args.output) 

    log_commands, log_results = open(args.output + "/commands.log", "w"), open(args.output + "/results.log", "w")  
    logit(log_commands, args)
    
    # 1. compile stage
    compile_command = COMMANDS['silo-paxos'] if args.enable_paxos == 1 else COMMANDS['silo-only']
    compile_command += " REPLAY_FOLLOWER=1" if args.enable_replay == 1 else " REPLAY_FOLLOWER=0"
    compile_command += " DBTEST_PROFILER=1" if args.enable_profiler == 1 else " DBTEST_PROFILER=0"

    compile_command += " > {tmp} 2>&1 \n".format(tmp=args.output + "/compile.log")
    logit(log_commands, compile_command) 

    if args.skip_compile == 0:
       if args.run == "RUN":
           call(compile_command, shell=True) 

    # 2. run over threads
    paxos_replica_config = "third-party/paxos/config/1c1s1p.yml" if args.replica_num == 1 else "third-party/paxos/config/1c1s1p-2r.yml"
    bulk_size = args.bulk_size

    for thread_num in range(args.start_thread, args.end_thread+1):
        tmp = args.output + "/run.{thread_num}.log".format(thread_num=thread_num)
        run_command = "./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor {thread_num} --num-threads {thread_num} --numa-memory 1G --parallel-loading --runtime {run_time} {force_cpu} -F {paxos_replica_config} -F third-party/paxos/config/occ_paxos.yml -S {bulk_size} > {tmp} 2>&1 \n".format(thread_num=thread_num, run_time=args.run_time, force_cpu='--bench-opts="--cpu-gap 1 --num-cpus 32"' if args.force_cpu == 1 else "", paxos_replica_config=paxos_replica_config, bulk_size=bulk_size, tmp=tmp)
        print(run_command)
        logit(log_commands, run_command)

        if args.run != "RUN":
            continue

        call(run_command, shell=True)
        isDONE = False
        with open(tmp, "r") as hanlder:
            for e in hanlder.readlines():
                if "agg_persist_throughput" in e:
                    isDONE = True 
                    throughput = float(e.replace("agg_persist_throughput:", "").replace("ops/sec", "").strip())
                    print("%s\t%s\n" % (thread_num, throughput))
                    logit(log_results, "%s\t%s" % (thread_num, throughput))
                    if args.enable_profiler == 1:
                        prof_command = "google-pprof --pdf ./out-perf.masstree/benchmarks/dbtest ./dbtest.prof > {output}".format(output=args.output + "/prof.%s.pdf" % thread_num)
                        call(prof_command, shell=True)
                        logit(log_commands, prof_command)

        if not isDONE:
            break
    log_results.close()
    log_commands.close()
    
    # https://gperftools.github.io/gperftools/heapprofile.html
    # make clean && make -j dbtest MODE=perf SERIALIZE=0 PAXOS_ENABLE_CONFIG=0 STO_BATCH_CONFIG=0 SILO_SERIAL_CONFIG=0 PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=0 OPTIMIZED_REPLAY=1 REPLAY_FOLLOWER=0 USE_MALLOC_MODE=2 DBTEST_PROFILER=1
    # env HEAP_PROFILE_INUSE_INTERVAL="10485760000" ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta --scale-factor 24 --num-threads 24 --numa-memory 1G --parallel-loading --runtime 30 --bench-opts="--cpu-gap 1 --num-cpus 32" -F third-party/paxos/config/1c1s1p.yml -F third-party/paxos/config/occ_paxos.yml -S 100
