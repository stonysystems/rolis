#!/usr/bin/env bash
. common.sh
CONFIG="1c2s6r1p.yml"
START_THREAD_COUNT=1
END_THREAD_COUNT=6
START_RUN_TIME=1
END_RUN_TIME=2
SERIALIZE="0"
PAXOS_BATCHING=0
STO_BATCHING=0
STO_CONFIG_TYPE=0
FILE_SUFFIX="no_serialize_runtime"

function goto_proot() {
    cd ${PARENT_DIR}
}

function goto_croot() {
    cd ${CURRENT_DIR}
}

function intro() {
    RUN_TIME=$1

    echo "Running ${CONFIG} with number of threads in range [${START_THREAD_COUNT},${END_THREAD_COUNT}] for ${RUN_TIME} seconds"
    if [[ SERIALIZE == "1" ]]; then
        echo "It's there."
    fi
}

function execute() {
    RUN_TIME=$1
    FILE_SUFFIX_PASSED="${FILE_SUFFIX}-$1-threads-${START_THREAD_COUNT}-${END_THREAD_COUNT}_${CONFIG}"
    python3 dbtest_build_and_save.py --output_file_suffix="${FILE_SUFFIX_PASSED}" --serialize="${SERIALIZE}" --paxos_site_config=third-party/paxos/config/"${CONFIG}" --paxos_batching="${PAXOS_BATCHING}" --sto_batching="${STO_BATCHING}" --sto_batching_config="${STO_CONFIG_TYPE}" --run_time="${RUN_TIME}" --thread_count_start="${START_THREAD_COUNT}" --thread_count_end="${END_THREAD_COUNT}"
}



goto_proot

for c in "${limits[@]}"
do
    intro $c
    execute $c
done

goto_croot

