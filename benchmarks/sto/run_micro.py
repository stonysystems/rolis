#!/usr/bin/env python

import subprocess
import numpy
import re
import json

def timeout_command(cmd, timeout):
    """call shell-command and either return its output or kill it
    if it doesn't normally exit within timeout seconds and return None"""
    import datetime, os, time, signal

    start = datetime.datetime.now() 
    process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    
    while process.poll() is None:
        time.sleep(0.1)
        now = datetime.datetime.now()
        if (now - start).seconds > timeout:
            os.kill(process.pid, signal.SIGKILL)
            os.waitpid(-1, os.WNOHANG)
            return None
    return process.stdout.read()

def run_experiment(cmd, t):
    time = 0.0
    for i in range(t):
        while True:
            out = timeout_command(cmd, 1000)
            g = re.search("(?<=real time: )[0-9]*\.[0-9]*", out)
            if g == None:
                print "retry " + str(cmd)
                continue
            out = g.group(0)
            time += float(out)
            #time.append(float(out))
            #timelib.sleep(1.0);
            break
    return time / t

def run_benchmark(cmd, nthreads):
    time_list = []
    for n in nthreads:
        cmd[3] = '--nthreads=' + str(n)
        time = run_experiment(cmd, t)
        time_list.append(time)
    return time_list


if __name__ == "__main__":
    make_cmd = ['make', 'concurrent-1M']
    subprocess.check_output(make_cmd)
    
    file = open("results.json", 'w')
    cmd = ['./concurrent-1M', '3', 'array-nonopaque', '--nthreads=1', '--ntrans=40000000', '--writepercent=1', '--opspertrans=50']
    nthreads = [1]
    n = 4
    while n < 64:
        nthreads.append(n)
        n = n + 4
    t = 5
    time = run_benchmark(cmd, nthreads)
    speedup = []
    for t in time:
        speedup.append(time[0] / t)
    out = {u"xlabel": "threads", u"ylabel": "time", u"nthreads": nthreads, u"time": time}
    file.write(json.dumps(out, indent=4))
    
