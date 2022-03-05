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

# ip of leader replica
server_ip = "10.1.0.7"
port = 23333

minCPU = int(sys.argv[1])
maxCPU = int(sys.argv[2])
flag = int(sys.argv[3])


killCommand = "sudo pkill -f dbtest"


if __name__ == "__main__":
    follower = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    follower.connect((server_ip, port))
    
    follower.recv(1024)  # received msg1 
    
    for cores in range(minCPU,maxCPU+1):
        os.system(killCommand)
        logging.info("succeed killing dbtest")
        follower.send("msg2".encode())
        follower.recv(1024) # received msg3
        # at this moment, the leader kill this dbtest
        logging.info("both machines have killed dbtest")
        follower.send("msg4".encode())
        time.sleep(0.5)
        logging.info("start to execute, cores: " + str(cores))
        os.system("sudo bash mc1.sh "+str(cores)+" "+str(flag))
        time.sleep(cores+30+10)
    follower.close()
    os.system(killCommand)
