
## Janus 
[![Build Status](https://travis-ci.org/NYU-NEWS/janus.svg?branch=master)](https://travis-ci.org/NYU-NEWS/janus)

Code repo for our OSDI '16 paper:
[Consolidating Concurrency Control and Consensus for Commits under Conflicts](http://mpaxos.com/pub/janus-osdi16.pdf)


## Quick start (with Ubuntu 16.04 or newer)

Install dependencies:

```
sudo apt-get update
sudo apt-get install -y \
    git \
    pkg-config \
    build-essential \
    clang \
    libapr1-dev libaprutil1-dev \
    libboost-all-dev \
    libyaml-cpp-dev \
    python3-dev \
    python3-pip \
    libgoogle-perftools-dev
sudo pip3 install -r requirements.txt
```

Get source code:
```
git clone --recursive https://github.com/NYU-NEWS/janus.git
```

Build:

```
python3 waf configure build -t

```

Test run:
```
python3 test_run.py -m janus
```

## More
Check out the doc directory to find more about how to build the system on older or newer distros, how to run the system in a distributed setup, and how to generate figures in the paper, etc.

## Test Benchamark [April-25-2020]

| mode           | site      | bench     | concurrent     | result 	 | time   | 
|----------------|-----------|:---------:|----------------|:--------:|:------:|
| janus          | 1c1s1p    | tpca      | concurrent_100 | OK     	 | 23.18s |
| janus          | 2c2s1p    | tpca      | concurrent_100 | OK     	 | 21.13s |
| janus          | 64c8s1p   | tpca      | concurrent_100 | OK     	 | 23.42s |
| janus          | 3c3s3r1p  | tpca      | concurrent_100 | OK     	 | 23.24s |
| janus          | 1c1s1p    | tpcc      | concurrent_100 | OK     	 | 26.25s |
| janus          | 2c2s1p    | tpcc      | concurrent_100 | OK     	 | 25.14s |
| janus          | 64c8s1p   | tpcc      | concurrent_100 | OK     	 | 37.72s |
| janus          | 3c3s3r1p  | tpcc      | concurrent_100 | OK     	 | 38.43s |

