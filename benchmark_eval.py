import os.path
from os import path
from subprocess import call, STDOUT, check_output
import time
import datetime
import argparse
import os
import subprocess, threading

# python3 benchmark_eval.py --start_thread 11 --end_thread 32
# ag 'agg_persist_throughput'

class Command(object):
    def __init__(self, cmd):
        self.cmd = cmd
        self.process = None

    def run(self, timeout):
        def target():
            print('Thread started: %s' % self.cmd)
            self.process = subprocess.Popen(self.cmd, shell=True)
            self.process.communicate()
            print('Thread finished')

        thread = threading.Thread(target=target)
        thread.start()

        thread.join(timeout)
        if thread.is_alive():
            print('Terminating process')
            self.process.terminate()
            thread.join()
        print(self.process.returncode)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""benchmark for evaluation""")
    parser.add_argument('--start_thread', default=1, type=int, help='default: 1')
    parser.add_argument('--end_thread', default=32, type=int, help='default: 32')

    args = parser.parse_args()

    call("sudo ulimit -n 10000", shell=True)

    for thread_num in range(args.start_thread, args.end_thread+1):
        run_command = "sudo ./multi.sh %s 1" % thread_num
        command = Command(run_command)
        command.run(timeout=60)
