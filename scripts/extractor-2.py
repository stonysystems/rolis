import os.path
from os import path
import sys


if __name__ == "__main__":

    arrs = [50, 100, 200, 400, 800, 1600, 3200, 6400]


    for batch in arrs:
        file_pattern = "./xxxx15/leader-15-{batch}.log".format(batch=batch)
        value = ""
        if path.exists(file_pattern):
            with open(file_pattern) as handler:
                lines = handler.readlines()
                for line in lines:
                    if "agg_persist_throughput" in line: # agg_persist_throughput: 624261 ops/sec
                        value += str(float(line.replace("agg_persist_throughput:", "").replace("ops/sec", ""))) + "\t"
                    if "10% latency:" in line:
                        value += str(float(line.replace("10% latency:", ""))) + "\t"
                    if "50% latency:" in line:
                        value += str(float(line.replace("50% latency:", ""))) + "\t"
                    if "90% latency:" in line:
                        value += str(float(line.replace("90% latency:", ""))) + "\t"
                    if "95% latency:" in line:
                        value += str(float(line.replace("95% latency:", ""))) + "\t"
                    if "99% latency:" in line:
                        value += str(float(line.replace("99% latency:", ""))) + "\t"
            print(value)
                        

                        

