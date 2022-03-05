#start executing this script under silo-sto directory, after leader.py
#you may want to change the server_ip and port variable before executing
#input: three integers. The minimal number of cores, the maximal number of cores and the flag

import logging
import os
import socket
import time
import sys

logging.basicConfig(
         format='%(asctime)s %(levelname)-8s %(message)s',
         level=logging.INFO)

lines = []
with open('./scripts/ip_leader_replica') as f:
    lines = f.readlines()
server_ip = lines[0].strip()  # leader ip

with open('./scripts/ip_p2_follower_replica') as f:
    lines = f.readlines()
server_ip2 = lines[0].strip()  # p2 follower ip

port = 23333

minCPU = int(sys.argv[1])
maxCPU = int(sys.argv[2])
flag = int(sys.argv[3])


killCommand = "sudo pkill -f dbtest"


if __name__ == "__main__":
    follower = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    follower.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1);
    follower.connect((server_ip, port))

    follower.recv(1024) # received msg1

    follower2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    follower2.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1);
    follower2.connect((server_ip2, port))

    follower2.recv(1024) # received msg2

    for cores in range(minCPU,maxCPU+1):
        logging.info("number of cores :" + str(cores))
        os.system(killCommand)
        logging.info("kill dbtest")
        follower.send("msg3".encode())
        follower2.send("msg4".encode())
        follower.recv(1024) # received msg5
        follower2.recv(1024) # received msg6
        # at this moment, three machines execute kill commmand

        follower.send("msg7".encode())
        time.sleep(1)
        cmd="sudo ./mb1.sh "+str(cores)
        logging.info("start to execute: " + cmd)
        os.system(cmd)
        time.sleep(1)
        follower2.send("msg8".encode())
        time.sleep(cores+30+30)
    follower.close()
    follower2.close()
    os.system(killCommand)

