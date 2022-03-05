import sys
import os

from collections import deque


def extract(line):
    t1 = line.find(':')
    t2 = line.rfind(':')
    return float(line[t1 + 1:t2].strip())


def every_file(file_name):
    ans = []
    with open(file_name) as fs:
        for line in fs.readlines():
            if 'XXX:' in line:
                ans.append(extract(line))
    ans1 = sorted(ans,reverse=False)[:90000]
    ans2 = sorted(ans, reverse=True)[:90000]
    if len(ans) == 0:
        print("Average DISK IO Throughput for File %25s is %s" % (file_name, 0))
        return ""

    print("File %20s with size %5.6s MB : Len %6s AL %.6s AM %.6s" % (file_name,
                                                                          os.stat(file_name).st_size/1000000.0,
                                                                          len(ans),sum(ans1) / len(ans1),
                                                                          sum(ans2) / len(ans2)))


def main(file_path):
    for each in os.listdir(file_path):
        if '.txt' in each:
            every_file(each)


if __name__ == '__main__':
    main(sys.argv[1])
    # main(".")
