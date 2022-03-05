import os
import sys
import getopt
import subprocess
import re
import time
from tqdm import tqdm
from sys import platform
import pandas as pd

CSV_REPORT_PATH = "Reports"
DEFAULT_SLEEP_TIME_IN_SECONDS = 0.1
executable_folder_full_path = "C:\\Users\\mrkumar\\Downloads\\BBReleases\\Harsh\\Release"
reports_folder = "Z:\\xbbxReports"
platform_as_codes = ""
cwd = os.getcwd()

os.chdir(executable_folder_full_path)

print("Running in the directory {}".format(os.getcwd()))


class PLATFORM:
    DARWIN = 1
    LINUX = 2
    WINDOWS = 3


if platform == "linux" or platform == "linux2":
    # linux
    platform_as_codes = PLATFORM.LINUX
elif platform == "darwin":
    # OS X
    platform_as_codes = PLATFORM.DARWIN
elif platform == "win32":
    # Windows...
    platform_as_codes = PLATFORM.WINDOWS
else:
    platform_as_codes = 0

assert platform_as_codes in [1, 2, 3]


def get_exe_fmt(cnd):
    if cnd == 3:
        return ".exe"
    else:
        return ""


dummy_dict = {"thread_count": "", "cmd1_result": "", "combined_cmd": "", "Algorithm_Name": ""}
rows = []
algo = {"rdsResults": "NA NUMA-aware algorithm",
        "fcResults": "FC Flat combining",
        "fcrwResults": "FC+ Flat combining with readers-writer lock",
        "rwlResults": "RWL One big readers-writer lock",
        "slResults": "SL One big lock (spinlock)",
        "lockResults": "LF Lock-free algorithm"}

# Global constants
serverCmd = 'redis-server{} --maxheap 200mb --wm 1 --exp-trials 3 --exp-duration-us 2000000'.format(
    get_exe_fmt(platform_as_codes))

serverCmdFCRW = 'redis-server-fcrw{} --maxheap 200mb --wm 1 --exp-trials 3 --exp-duration-us 2000000'.format(
    get_exe_fmt(platform_as_codes))

serverCmdRWL = 'redis-server-rwl{} --maxheap 200mb --wm 1 --exp-trials 3 --exp-duration-us 2000000'.format(
    get_exe_fmt(platform_as_codes))

serverCmdSL = 'redis-server-sl{} --maxheap 200mb --wm 1 --exp-trials 3 --exp-duration-us 2000000'.format(
    get_exe_fmt(platform_as_codes))

clientCmd = 'redis-benchmark{} -t zadd -c 100 -P 100 -n 200000 -r 10000 -q'.format(get_exe_fmt(platform_as_codes))

# .\redis-benchmark.exe -t zadd -c 100 -P 100 -n 200000 -r 10000 -q ; .\redis-benchmark.exe -t zmixed -c 150 -P 1 -n 2000 -r 10000 -f 50 -q

clientCmd2 = 'redis-benchmark{0} -t zadd -c 100 -P 100 -n 200000 -r 10000 -q ; redis-benchmark{0} -t zmixed -c 250 -P ' \
             '1 -n 3000 -r 10000 -f 50 -q'.format(get_exe_fmt(platform_as_codes))


def errorExit(error):
    print(error + "\n")
    sys.exit(1)


def usage():
    usageStr = "Usage: \n\n"
    usageStr = usageStr + "python ./rds-run-exps.py\n"
    return usageStr


# Runs redis clients in a blocking manner
def runClient():
    # print("-- START --")
    cli1 = subprocess.Popen(clientCmd.split(),
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)

    # cli1 = subprocess.run(clientCmd.split(), shell=False, stdout=subprocess.PIPE,
    #                         stderr=subprocess.PIPE)
    # Sleep for a bit to let the redis db settle
    time.sleep(DEFAULT_SLEEP_TIME_IN_SECONDS)
    # cli = subprocess.call('sleep 0.1'.split(), shell=False)
    # os.system(clientCmd2)
    cli2 = subprocess.Popen(clientCmd2.split(),
                            stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT)
    # cli2 = subprocess.call(clientCmd2.split(), shell=False)
    # print("-- END --")
    return cli1, cli2


# Runs redis server in a non-blocking way, returns a handle to the subprocess
def runServer(cmd, threads, mode):
    cmd = cmd + ' --threads ' + str(threads + 1) + ' ' + mode
    print("Running command: " + cmd)
    return subprocess.Popen(cmd.split(),
                            shell=True,
                            stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE)


def updateStats(thread_num, threads, results, output):
    output = str(output)
    nums = re.findall("\d+\.\d+ requests per second", output)
    # readRatios = re.findall("read ratio = \d+\.\d+", output)

    i = 0
    for _ in nums:
        r = thread_num  # ratio.split()[-1]

        if r not in results:
            results[r] = [[thread_num, nums[i].split()[0]]]
        else:
            results[r].append([thread_num, nums[i].split()[0]])
        i += 1


def printStats(threadCnts, results, case_str):
    print('-------------------------')
    print('    Algorithm {0}   '.format(case_str))
    print('-------------------------')
    print('\t\t   #(ThreadCount)')
    for each_thread_cnt in threadCnts:
        res = results.get(each_thread_cnt, [])
        res_as_list = list(map(lambda x: x[1], res))
        result_in_fmt = "\t".join(res_as_list)
        dict_copy = dummy_dict.copy()
        dict_copy["thread_count"] = each_thread_cnt
        dict_copy["cmd1_result"] = res_as_list[0]
        dict_copy["combined_cmd"] = res_as_list[1]
        dict_copy["Algorithm_Name"] = case_str.split(" ")[0]
        rows.append(dict_copy)
        print("Statistics     \t\t{0}\t\t{1}".format(each_thread_cnt, result_in_fmt))
    print('---' * 10)


def printStats_org(threadCnts, results):
    header = 'Threads\t'
    readRatios = sorted(results);
    # Build and print the table header
    for ratio in readRatios:
        header += ratio + '\t'
    print(header)
    i = 0
    for threads in threadCnts:
        line = str(threads) + '\t'
        for ratio in readRatios:
            assert (results[ratio][i][0] == int(threads))
            line += str(results[ratio][i][1]) + '\t'
        print(line)
        i += 1
    print('\n')


def main(argv):
    # Parse the command-line arguments.
    try:
        opts, args = getopt.getopt(argv, " ", [])
    except getopt.GetoptError:
        errorExit("Error parsing command line options\n" + usage())

    for opt, arg in opts:
        pass
        # if opt == "-a"
        # else:
        #    errorExit("Unexpected command line argument " + opt + "\n" + usage())

    threadCnts = [1, 3, 6, 9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 60, 70 , 80, 90, 100, 110, 112]
    # threadCnts = [1]
    throughputP = re.compile('\"(.+?)\".*?\"et\":\"(.+?)\"')

    rdsResults = {}
    fcResults = {}
    fcrwResults = {}
    rwlResults = {}
    slResults = {}
    lockResults = {}

    def process_both_cmds(first, second, dict_op, thread_num):
        stdout, stderr = first.communicate()
        updateStats(thread_num, threads, dict_op, stdout)

        stdout, stderr = two.communicate()
        updateStats(thread_num, threads, dict_op, stdout)

    for threads in tqdm(threadCnts):
        print("\n\n")
        rs = runServer(serverCmd, threads, '--repl')
        # This call blocks; when its done, we know the server is done too
        one, two = runClient()
        process_both_cmds(one, two, rdsResults, threads)

        rs = runServer(serverCmd, threads, '--fc')
        # This call blocks; when its done, we know the server is done too
        one, two = runClient()
        process_both_cmds(one, two, fcResults, threads)

        rs = runServer(serverCmdFCRW, threads, '--fc')
        # This call blocks; when its done, we know the server is done too
        one, two = runClient()
        process_both_cmds(one, two, fcrwResults, threads)

        rs = runServer(serverCmdRWL, threads, '--fc')
        # This call blocks; when its done, we know the server is done too
        one, two = runClient()
        process_both_cmds(one, two, rwlResults, threads)

        rs = runServer(serverCmdSL, threads, '--fc')
        # This call blocks; when its done, we know the server is done too
        one, two = runClient()
        process_both_cmds(one, two, slResults, threads)

        rs = runServer(serverCmd, threads, '')
        # This call blocks; when its done, we know the server is done too
        one, two = runClient()
        process_both_cmds(one, two, lockResults, threads)
        print("\n\n")

    # Print results
    print("\n=============== RESULTS ===============\n\n")
    print("RDS results:\n")
    printStats(threadCnts, rdsResults, algo["rdsResults"])
    print("FC results:\n")
    printStats(threadCnts, fcResults, algo["fcResults"])
    print("FCRW results:\n")
    printStats(threadCnts, fcrwResults, algo["fcrwResults"])
    print("RWL results:\n")
    printStats(threadCnts, rwlResults, algo["rwlResults"])
    print("SL results:\n")
    printStats(threadCnts, slResults, algo["slResults"])
    print("Lock results:\n")
    printStats(threadCnts, lockResults, algo["lockResults"])

    ts = time.gmtime()

    csv_saver_fmt = time.strftime("%m-%d-%Y-Time-%H-%M-%S", ts)
    os.chdir(reports_folder)
    os.makedirs(CSV_REPORT_PATH, exist_ok=True)
    df = pd.DataFrame(rows)
    df.to_csv("{parent_path}{sep}Report_{fmt}.csv".format(parent_path=CSV_REPORT_PATH, sep=os.sep, fmt=csv_saver_fmt),
              index=False)


if __name__ == "__main__":
    # Note this strips off the command name.
    main(sys.argv[1:])
