Welcome to the Rolis artifact for our Eurosys'22 submission. 

_Rolis: a software approach to efficiently replicating multi-core transactions_

## Project organization
 - `benchmarks`
   - `benchmarks/sto`: Transaction control, serialization (in `Transactions.cc`) and replay interface implementation (in `ReplayDB.cc`)
   - `benchmarks/tpcc.cc`: TPC-C benchmark implementation
   - `benchmarks/micro_bench.cc`: YCSB++ benchmark implementation
 - `docker`: the docker container
 - `scripts`: several experimental scripts and scripts to plot figures in the paper
 - `third-party`: the dependencies on MultiPaxos ported from Janus with the leader election
 - `dbtest.cc`: the `main()` portal to start the program, including replay logic

 You can find a more detailed document in [modules](./documents/code.md).

## Run the experiment locally
In this section, you can set up Rolis locally for testing and verification, which is mainly for badges `Artifacts Available` and `Artifacts Evaluated - Functional`.

### Obtain a Docker container
We run all our codes on `ubuntu 18.04` which mainly depends on several Linux libraries (i.e., boost, gcc and libyaml-cpp-dev). We provide a docker image with all required dependencies and source code for ease **so you can run on any local machine supporting Docker**. You can directly build a runnable Rolis instance with all required dependencies and source code via
```bash
cd ~
git clone https://github.com/stonysystems/rolis.git
cd ~/rolis
cd ./docker
bash docker.sh
```

### Minimal working examples locally
You can start Rolis instance locally (*using different processes to mimic actual distributed environment*) to verify the functionability of the program through three minimal experiments: 
 * two running replicas without failure; 
 * Silo only;
 * three replicas with failure;

```bash
# 1. enter the docker container
docker exec -it ubuntu_eurosys2022 /bin/bash

# 2. run 3 minimal experiments (inside the container): 
make paxos
cd ./test && bash ./runner.sh
```

We should observe status "PASS" in green at the end of experiments as below
![alter](./documents/minimal_exp.PNG)

## Run the expriment in actual distributed environment
Before moving on, please make distributed environment ready in [environment](./env.md). 


We provide one-click runnable to generate all results under folder `./results`. It would take up to 2 ~ 3 hours to run all experiments.
```bash
# on the leader replica
cd ~/rolis
bash one-click.sh
```

## Comparisons
### Calvin 
Please note that, you need a 32-core machine (*we eliminate the replication in this Calvin implementation*). All results you can find under `./results`.
```
cd ~
git clone https://github.com/shenweihai1/star.git
bash one-click.sh
```
### 2PL
In this experiment, you need 32 4-core clients and 3 32-core servers (*Please make sure all clients and servers can connect to each other without password*).
#### environment setup
1. config IPs and username for clients and servers in `batch_janus.sh`
2. then download code and install dependencies
```
# on the leader replica
cd ~
git clone https://github.com/shenweihai1/janus.git
cd janus
./batch_janus.sh scp
./batch_janus.sh install
```
3. config Paxos configuration under `par-dist`. 
```
p1 => leader ip
p2 => p1 replica ip
p3 => p2 replica ip
c1 ~ c32 => client1 ~ client32 ip
```
4. compile and sync to code to all machines
```
cd ~/janus
make 
./batch_janus.sh scp
```

#### Run the experiment. 
All results you can find under `./results` (*We pick the one whose progress is closest to 100% as the throughput*)
```
cd ~/janus
bash one-click.sh
```
### case 1: run chosen experiments
The one-click script usually can complete all experiments till the end, but everything can happen. In this case, you don't have to re-run completed experiments. You can comment the unwanted experiments and leave the wanted ones in the `one-click.sh`. For example, you can choose to run `experiment-9` and `experiment-12` as below
![alt](./documents/one-click.png)

### case 2: run the experiment with chosen worker threads
You can choose to re-run the experiment with chosen worker threads, e.g., only run with worker threads 13-15 by configuration in the `one-click.sh`.
![alt](./documents/rerun-exp.png)
You can find a more detailed experiment instructions in [experiments](./instructions.md).

The results should be as below
1. `batch-latency.log`: the 50th, 90th and 95th latency results of Rolis with different batch-sizes
2. `batch-throughput.log`: the throughput results of Rolis with different batch-sizes
3. `failover-16-throughput.log`: the throughput over time in the failover scenario
4. `scalability-tpcc.log`: the throughput of Rolis with different worker threads under tpcc workload
5. `scalability-ycsb.log`: the throughput of Rolis with different worker threaders under ycsb++ workload
6. `silo-only-tpcc.log`: the throughput of Silo under tpcc workload
7. `silo-only-ycsb.log`: the throughput of Silo under ycsb++ workload
8. `single_paxos_stream.log`: single stream experiment
9. `throughput_skewed_workload-rolis.log`: Rolis's throughput on skewed-workload
10. `silo-only-skewed.log`: Silo's throughput on skewed-workload
11. `replay-only.log`: Replay-only experiment