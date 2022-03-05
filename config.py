class BASE:
    NUMA = "4G"
    CONST_CLEAN = "make clean"
    RUN_TIME = 30
    MAX_THREAD_COUNT = 30
    REPLAY_CONST_THREAD_COUNT = 30
    REPLAY_LOG_FILE_EXT = ".txt"
    # "/home/anshkhanna720/projects/silo-sto"
    CONST_DIR_PREFIX = "/home/mrityunjaykumar/projects/silo-sto"
    CONST_REPLAY_READER_PATH = CONST_DIR_PREFIX+"/prev_logs/"
    CONST_REPLAY_PERF_PATH = CONST_DIR_PREFIX+"/prev_logs/replay_logs/1/"
    CONST_RESULT_SAVER_PATH = CONST_DIR_PREFIX+"/STO_RESULTS/result_{suffix}.csv"

    def get_all_params(self):
        vals = [str(i) for i in dir(self) if i.startswith("CONST_")]
        return {k: getattr(self, k) for k in vals}

    def build_build_command_prepare(self):
        pass

    def build_run_command_prepare(self, *args, **kwargs):
        pass


class SERIALIZE(BASE):
    def __init__(self):
        self.CONST_BUILD_COMMAND = ""
        self.BUILD_CMD = "MODE=perf SERIALIZE={SERIALIZE} REPLAY_FOLLOWER={REPLAY_FOLLOWER} STO_BATCH_CONFIG={STO_BATCH_CONFIG} PLOT_WRITE_SPEED={PLOT_WRITE_SPEED} make -j dbtest"
        self.CONST_CLEAN = "make write_clean"
        self.BENCH_MARK_TYPE = "tpcc"
        self.DB_TYPE = "mbta"
        self.RESULT_FILE_NAME = "file_THREAD_{}_SCALE_{}.txt"
        """
        ./out-perf.masstree/benchmarks/dbtest --verbose --bench tpcc --db-type mbta " \
                                 "--scale-factor 9 --num-threads 9 --numa-memory 4G " \
                                 "--parallel-loading --runtime 30
        """
        self.CONST_RUN_COMMAND = "./out-perf.masstree/benchmarks/dbtest --verbose --bench {0} --db-type {1} " \
                                 "--scale-factor {SCALE} --num-threads {THREAD_COUNT} -S {STO_BATCHING} -A {PAXOS_BATCHING} --numa-memory {2} " \
                                 "--parallel-loading --runtime {RUN_TIME}  -F {PAXOS_SITE_CONFIG} -F third-party/paxos/config/occ_paxos.yml"

    def build_build_command_prepare(self, serialize=0, plot_write_speed=0, replay_follower=0, sto_batching_config=0):

        self.CONST_BUILD_COMMAND = self.BUILD_CMD.format(SERIALIZE=serialize,
                                                         PLOT_WRITE_SPEED=plot_write_speed,
                                                         REPLAY_FOLLOWER=replay_follower,
                                                         STO_BATCH_CONFIG=sto_batching_config)

    def build_run_command_prepare(self,run_time):
        if run_time == 0:
            run_time = self.RUN_TIME
        self.CONST_RUN_COMMAND = self.CONST_RUN_COMMAND.format(self.BENCH_MARK_TYPE, self.DB_TYPE, self.NUMA,
                                                               RUN_TIME=run_time,
                                                               STO_BATCHING="{STO_BATCHING}",
                                                               PAXOS_SITE_CONFIG="{PAXOS_SITE_CONFIG}",
                                                               PAXOS_BATCHING="{PAXOS_BATCHING}",
                                                               SCALE="{SCALE}",
                                                               THREAD_COUNT="{THREAD_COUNT}")


class SEARIALIZE_A(SERIALIZE):
    def __init__(self):
        SERIALIZE.__init__(self)
        self.CONST_SCALE_AND_THREADS = [(i + 1, i + 1) for i in range(BASE.MAX_THREAD_COUNT)]

    def update_scale_threads(self, start=1, end=2, scale=0):
        self.CONST_SCALE_AND_THREADS = []
        temp = start
        while temp <= end:
            self.CONST_SCALE_AND_THREADS.append((temp, temp))
            temp+=1


class SEARIALIZE_B(SERIALIZE):
    def __init__(self):
        SERIALIZE.__init__(self)
        CONSTANT_SCALE = 25
        self.CONST_SCALE_AND_THREADS = [(i + 1, CONSTANT_SCALE) for i in range(self.MAX_THREAD_COUNT)]

    def update_scale_threads(self, start=1, end=2, scale=25):
        self.CONST_SCALE_AND_THREADS = []
        temp = start
        while temp <= end:
            self.CONST_SCALE_AND_THREADS.append((temp, scale))
            temp+=1


class REPLAY(BASE):
    CONST_BUILD_COMMAND = "MODE=perf PLOT_REPLAY_SPEED={PLOT_REPLAY_SPEED} make -j ht_mt2"
    CONST_CLEAN = "make replay_clean"
    RUN_COMMAND = "./out-perf.masstree/benchmarks/ht_mt2 --file-count {file_count} --num-threads {num_threads}"
    CONST_RUN_COMMAND = "./out-perf.masstree/benchmarks/ht_mt2 --file-count {file_count} --num-threads {num_threads}"

    def build_run_command_prepare(self, **kwargs):
        file_count = kwargs.get("file_count", 1)
        thread_count = kwargs.get("thread_count", 1)
        CONST_RUN_COMMAND = self.RUN_COMMAND.format(file_count=file_count,
                                                    num_threads=thread_count)
        return CONST_RUN_COMMAND


if __name__ == '__main__':
    b = BASE()
    params = b.get_all_params()
