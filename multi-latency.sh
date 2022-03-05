#!/bin/bash
# ulimit -n 8000
# ----------------------------------------------------------------------------- compile ------------------------------------------------------------------------------------------

sudo pkill -f dbtest
# make paxos
make clean && make -j dbtest MODE=perf \
                      SERIALIZE=1 PAXOS_ENABLE_CONFIG=1 \
                      STO_BATCH_CONFIG=2 SILO_SERIAL_CONFIG=0 \
                      PAXOS_ZERO_CONFIG=0 LOGGING_TO_ONLY_FILE=0 \
                      OPTIMIZED_REPLAY=1 REPLAY_FOLLOWER=1 \
                      DBTEST_PROFILER=0 TRACKER_LATENCY=1