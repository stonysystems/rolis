import psutil
import time
import sys
import getopt
import datetime

# To support Memory recording and CPU recording

"""
purpose:
  monitor CPU and mem usage per [interval] second

usage:
  python3 mem_monitor.py -o [outputfile] -d [interval] -n [numa-required], i.e., python3 mem_monitor.py -o m.log -d 0.5 -n 1

references:
  1. https://peteris.rocks/blog/htop/
  2. https://psutil.readthedocs.io/en/latest/
  3. http://manpages.ubuntu.com/manpages/bionic/man8/numastat.8.html
"""

interval = 0.5
outputFile = "./monitor.log"
ONLY_NUMA = False
numa_num = 1 # numactl --hardware

def readNumaMem():
    ans = [0] * numa_num
    for i in range(numa_num):
        with open("/sys/devices/system/node/node%s/meminfo" % i) as handler:
            for l in handler.readlines():
                if "MemFree" in l:
                    w = int(l.replace("Node %s MemFree:" % i, "").replace("kB", ""))
                    ans[i] = round(w / 1024 / 1024.0, 2)  # G
                    break
    return ans


if __name__ == "__main__":
    argv = sys.argv[1:]

    try:
        opts, args = getopt.getopt(argv,"hd:o:n:", ["interval=", "ofile=", "numa="])
    except getopt.GetoptError:
        print('python3 mem_monitor.py -o [outputfile] -d [interval]')
        sys.exit(2)

    for opt, arg in opts:
        if opt == '-h':
            print('python3 mem_monitor.py -o [outputfile] -d [interval]')
            sys.exit()
        elif opt in ("-o", "--ofile"):
            outputFile = arg
        elif opt in ("-d", "--interval"):
            interval = float(arg)
        elif opt in ("-n", "--numa"):
            ONLY_NUMA = bool(arg)

    w = open("./" + outputFile, 'w')
    header = ""
    while True:
        cpus = psutil.cpu_percent(percpu=True)
        if header == "":
            header = "time\tmem_usage"
            for i in range(numa_num):
                header += "\tnode%s" % i

            if not ONLY_NUMA:
                for i in range(len(cpus)):
                    header += "\tcpu%s" % i
            w.write(header + "\n")

        # get the numbers
        mem = dict(psutil.virtual_memory()._asdict())['used']/1024/1024/1024
        #row = str(datetime.datetime.now())
        row = str(int(time.time()))
        row += "\t" + str(round(mem, 2))
        for m in readNumaMem():
            row += "\t" + str(m)
        if not ONLY_NUMA:
            for c in cpus:
                row += "\t" + str(c)
        w.write(row + "\n")
        print(row)
        w.flush()
        time.sleep(interval)
