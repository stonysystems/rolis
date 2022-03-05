#start executing this script under silo-sto directory, before follower.py
#you may want to change the server and port variable before executing
#input: three integers. The minimal number of cores, the maximal number of cores and the flag

import logging
import os
import socket
import time
import sys

logging.basicConfig(
         format='%(asctime)s %(levelname)-8s %(message)s',
         level=logging.INFO)

server_ip = "0.0.0.0"
port = 23333

minCPU = int(sys.argv[1])
maxCPU = int(sys.argv[2])
flag = int(sys.argv[3])

killCommand = "sudo pkill -f dbtest"

if __name__ == "__main__":
    server = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
    server.bind((server_ip,port))
    server.listen(5)
    connection, address = server.accept()
    
    connection.send("msg1".encode())
    
    for cores in range(minCPU,maxCPU+1):
        connection.recv(1024) # received msg3
        logging.info("number of cores :" + str(cores))
        logging.info("start to kill dbtest")
        os.system(killCommand)
        logging.info("kill dbtest")
        connection.send("msg6".encode())
        connection.recv(1024) # received msg7
        cmd="sudo ./sb0.sh "+str(cores)
        logging.info("start to execute: " + cmd)
        os.system(cmd)
    time.sleep(maxCPU+30+30)
    connection.close()
    server.close()
    os.system(killCommand)
