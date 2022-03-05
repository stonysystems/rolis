## Actual distributed environment setup

### Hardware preparation
To reproduce the results in Rolis, we need
- 3 server machines, each machine has 32 CPU cores running on `Ubuntu 18.04` within a single socket. (***Please ensure that 3 machines can connect to each other via `ssh` and share the same username, because our tool depends on `scp` and `ssh` to simplify the procedure**, which means you can connect to any other two machines on any machine through `ssh ip` directly without username required*)
- obtain the IP of 3 machines.

### Download code and install dependencies
Let's assume here, `10.1.0.7` is the leader replica, `10.1.0.8` serves as the p1 follower replica and `10.1.0.9` serves as the p2 follower replica. Run all following commands on the leader replica.
```bash
# on the leader replica 
cd ~
git clone https://github.com/shenweihai1/rolis-eurosys2022.git
cd rolis-eurosys2022
# install dependencies
bash ./install.sh

# config IPs for configuration 1.yml ~ 32.yml
cd ./third-party/paxos/config/1silo_1paxos_2follower

# ./run.sh {leader ip} {p1 follower ip} {p2 follower ip}
./run.sh "10.1.0.7" "10.1.0.8" "10.1.0.9"

# compile Paxos
cd ~/rolis-eurosys2022
make paxos
```

### Sync modifications to two other replicas via ssh (still run this command the leader replica)
```bash
cd ~/rolis-eurosys2022
bash ./batch_silo.sh scp
```
At this moment, the running environment on 3 replicas is ready. 