-include config.mk

### Options ###

DEBUG ?= 0
CHECK_INVARIANTS ?= 0

# 0 = libc malloc
# 1 = jemalloc
# 2 = tcmalloc
# 3 = flow
USE_MALLOC_MODE ?= 1

# Available modes
#   * perf
#   * backoff
#   * factor-gc
#   * factor-gc-nowriteinplace
#   * factor-fake-compression
#   * sandbox
MODE ?= perf
SERIALIZE ?= 0
CPU_PROFILING ?= 0
DBTEST_PROFILER ?= 0
PLOT_REPLAY_SPEED ?=0
PLOT_WRITE_SPEED ?=0
# enable the replay, for replay callback function definition etc
REPLAY_FOLLOWER ?= 0
PAXOS_INTERCEPT ?= 0
STO_BATCH_CONFIG ?= 0
SILO_SERIAL_CONFIG ?= 0
PAXOS_ZERO_CONFIG ?= 0
SILO_CID_CONFIG ?= 1
SILO_ALLOC_MALLOC ?= 1
PAXOS_ENABLE_CONFIG ?= 0
NETWORK_CLIENT_CONFIG ?= 0
NETWORK_CLIENT_CONFIG_YCSB ?= 0
OPTIMIZED_REPLAY ?= 0
LOGGING_TO_ONLY_FILE ?= 0
TRACKER_LATENCY ?= 0
FAIL_OVER_CONFIG ?= 0
FAIL_OVER_VARIABLE_CONFIG ?= 0
SINGLE_PAXOS ?= 0
NO_ADD_LOG_TO_NC ?= 0
# the REPLAY_FOLLOWER must to be enabled, then we can implement the replay callback function with empty implementation 
DISABLE_REPLAY ?= 0
SKEWED_WORKLOAD ?= 0
LOG_SUB_DIR ?= GEN_LOGS

THREAD_COUNT_S=$(strip $(THREADS))
OPTIMIZED_REPLAY_S=$(strip $(OPTIMIZED_REPLAY))
LOG_SUB_DIR_S=$(strip $(LOG_SUB_DIR))
LHOME=/dev/shm
LOG_FOLDER=$(LHOME)/GLOGS/
REPLAY_LOG_FOLDER=$(LHOME)/GLOGS/$(LOG_SUB_DIR_S)/prev_logs/
REPLAY_STATS_LOG_FOLDER=$(LHOME)/GLOGS/$(LOG_SUB_DIR_S)/replay_logs/

STO_RMW ?= 0
HASHTABLE ?= 0
GPROF ?= 0

CC ?= gcc
CXX ?= c++

###############

DEBUG_S=$(strip $(DEBUG))
CHECK_INVARIANTS_S=$(strip $(CHECK_INVARIANTS))
EVENT_COUNTERS_S=$(strip $(EVENT_COUNTERS))
USE_MALLOC_MODE_S=$(strip $(USE_MALLOC_MODE))
MODE_S=$(strip $(MODE))
STO_RMW_S=$(strip $(STO_RMW))
HASHTABLE_S=$(strip $(HASHTABLE))
GPROF_S=$(strip $(GPROF))
MASSTREE_CONFIG:=CC="$(CC)" CXX="$(CXX)" --enable-max-key-len=1024
SERIALIZE_S=$(strip $(SERIALIZE))
DBTEST_PROFILER_S=$(strip $(DBTEST_PROFILER))
PROFILE_S=$(strip $(CPU_PROFILING))
PLOT_REPLAY_SPEED_S=$(strip $(PLOT_REPLAY_SPEED))
PLOT_WRITE_SPEED_S=$(strip $(PLOT_WRITE_SPEED))
REPLAY_FOLLOWER_S=$(strip $(REPLAY_FOLLOWER))
PAXOS_INTERCEPT_S=$(strip $(PAXOS_INTERCEPT))
LOGGING_TO_ONLY_FILE_S=$(strip $(LOGGING_TO_ONLY_FILE))
TRACKER_LATENCY_S=$(strip $(TRACKER_LATENCY))
SINGLE_PAXOS_S=$(strip $(SINGLE_PAXOS))
NO_ADD_LOG_TO_NC_S=$(strip $(NO_ADD_LOG_TO_NC))
DISABLE_REPLAY_S=$(strip $(DISABLE_REPLAY))
SKEWED_WORKLOAD_S=$(strip $(SKEWED_WORKLOAD))
STO_BATCH_ENABLE_S=$(strip $(STO_BATCH_CONFIG))
SILO_SERIAL_CONFIG_S=$(strip $(SILO_SERIAL_CONFIG))
SILO_CID_CONFIG_S=$(strip $(SILO_CID_CONFIG))
SILO_ALLOC_MALLOC_S=$(strip $(SILO_ALLOC_MALLOC))
PAXOS_ENABLE_S=$(strip $(PAXOS_ENABLE_CONFIG))
NETWORK_CLIENT_CONFIG_S=$(strip $(NETWORK_CLIENT_CONFIG))
NETWORK_CLIENT_CONFIG_YCSB_S=$(strip $(NETWORK_CLIENT_CONFIG_YCSB))
FAIL_OVER_CONFIG_S=$(strip $(FAIL_OVER_CONFIG))
FAIL_OVER_VARIABLE_CONFIG_S=$(strip $(FAIL_OVER_VARIABLE_CONFIG))
PAXOS_ZERO_CONFIG_S=$(strip $(PAXOS_ZERO_CONFIG))

ifeq ($(DEBUG_S),1)
	OSUFFIX_D=.debug
	MASSTREE_CONFIG+=--enable-assertions
else
	MASSTREE_CONFIG+=--disable-assertions
endif
ifeq ($(CHECK_INVARIANTS_S),1)
	OSUFFIX_S=.check
	MASSTREE_CONFIG+=--enable-invariants --enable-preconditions
else
	MASSTREE_CONFIG+=--disable-invariants --disable-preconditions
endif
ifeq ($(EVENT_COUNTERS_S),1)
	OSUFFIX_E=.ectrs
endif
ifeq ($(STO_RMW_S),1)
	OSUFFIX_R=.rmw
endif
ifeq ($(HASHTABLE_S),1)
        OSUFFIX_H=.ht
endif
OSUFFIX=$(OSUFFIX_D)$(OSUFFIX_S)$(OSUFFIX_E)$(OSUFFIX_H)$(OSUFFIX_R)

ifeq ($(MODE_S),perf)
	O := out-perf$(OSUFFIX)
	CONFIG_H = config/config-perf.h
else ifeq ($(MODE_S),backoff)
	O := out-backoff$(OSUFFIX)
	CONFIG_H = config/config-backoff.h
else ifeq ($(MODE_S),factor-gc)
	O := out-factor-gc$(OSUFFIX)
	CONFIG_H = config/config-factor-gc.h
else ifeq ($(MODE_S),factor-gc-nowriteinplace)
	O := out-factor-gc-nowriteinplace$(OSUFFIX)
	CONFIG_H = config/config-factor-gc-nowriteinplace.h
else ifeq ($(MODE_S),factor-fake-compression)
	O := out-factor-fake-compression$(OSUFFIX)
	CONFIG_H = config/config-factor-fake-compression.h
else ifeq ($(MODE_S),sandbox)
	O := out-sandbox$(OSUFFIX)
	CONFIG_H = config/config-sandbox.h
else
	$(error invalid mode)
endif

#LOG_FLAGS
# https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
CXXFLAGS := -g  -std=gnu++1z -Wall -Wno-unused -Wno-sign-compare
CXXFLAGS += -MD -MP -Ithird-party/lz4 -Ithird-party/paxos/src -DCONFIG_H=\"$(CONFIG_H)\"
ifeq ($(GPROF_S),1)
	CXXFLAGS += -pg -static-libstdc++ -static-libgcc
endif

ifeq ($(DEBUG_S),1)
        CXXFLAGS += -fno-omit-frame-pointer -DDEBUG
else
        CXXFLAGS += -O2 #-funroll-loops -fno-omit-frame-pointer, O0 is not optimized
endif

ifeq ($(SILO_CID_CONFIG),0)
	CXXFLAGS += -DSILO_NO_FETCH_CID
endif

ifeq ($(OPTIMIZED_REPLAY_S),1)
	#CXXFLAGS += -DOPTIMIZED_VERSION_V2 Disabled by default
	CXXFLAGS += -DOPTIMIZED_VERSION
	CXXFLAGS += -DBULK_POOL_LOG_SUBMIT
	LOG_FOLDER=$(LHOME)/GLOGS/OPTIMAL/
endif

ifeq ($(SILO_SERIAL_CONFIG_S),1)
	CXXFLAGS += -DSILO_SERIAL_ONLY
endif

ifeq ($(PAXOS_ZERO_CONFIG_S),1)
	CXXFLAGS += -DPAXOS_ZERO_PUSH
endif

ifeq ($(FAIL_OVER_CONFIG_S),1)
  CXXFLAGS += -DFAIL_OVER
endif

ifeq ($(FAIL_OVER_VARIABLE_CONFIG_S),1)
  CXXFLAGS += -DFAIL_OVER_VARIABLE
endif

ifeq ($(PAXOS_ENABLE_S),1)
    CXXFLAGS += -DSERIALIZE_FEATURE
# benchmark flag - disabled by default
	CXXFLAGS += -DPAXOS_LIB_ENABLED
    #CXXFLAGS += -DBENCHMARK_TIME
    CXXFLAGS += -DALLOW_WAIT_AT_PAXOS_END
# re-enable this if only needed - @depricated now
#CXXFLAGS += -DALLOW_WAIT_AFTER_EACH_SUBMIT
endif

ifeq ($(NETWORK_CLIENT_CONFIG_S),1)
    CXXFLAGS += -DNETWORK_CLIENT
endif

ifeq ($(NETWORK_CLIENT_CONFIG_YCSB_S),1)
    CXXFLAGS += -DNETWORK_CLIENT_YCSB
endif

ifeq ($(SERIALIZE_S),1)
    CXXFLAGS += -DSERIALIZE_FEATURE
endif

ifeq ($(DBTEST_PROFILER_S),1)
    CXXFLAGS += -DDBTEST_PROFILER_ENABLED
endif


ifeq ($(STO_BATCH_ENABLE_S),0)
    CXXFLAGS += -DREGULAR_LOG_SUBMIT
endif

ifeq ($(STO_BATCH_ENABLE_S),1)
    CXXFLAGS += -DBULK_SIMPLE_LOG_SUBMIT
endif

ifeq ($(STO_BATCH_ENABLE_S),2)
    CXXFLAGS += -DBULK_POOL_LOG_SUBMIT
endif


ifeq ($(PROFILE_S),1)
    CXXFLAGS += -DCPU_PROFILING_FEATURE
endif

CXXFLAGS += -DLOG_FOLDER=\"$(LOG_FOLDER)\"
LOG_FLAGS += -DLOG_FOLDER=\"$(LOG_FOLDER)\"

ifeq ($(PLOT_REPLAY_SPEED_S),1)
    CXXFLAGS += -DTEST_REPLAY_SPEED_FEAT
    CXXFLAGS += -DLOG_FOLDER=\"$(REPLAY_LOG_FOLDER)\"
    LOG_FLAGS += -DLOG_FOLDER=\"$(REPLAY_LOG_FOLDER)\"
    LOG_FLAGS += -DTEST_REPLAY_SPEED_FEAT
endif

ifeq ($(PLOT_WRITE_SPEED_S),1)
    CXXFLAGS += -DTEST_WRITE_SPEED_FEAT
    CXXFLAGS += -DLOG_FOLDER=\"$(LOG_FOLDER)\"
    LOG_FLAGS += -DLOG_FOLDER=\"$(LOG_FOLDER)\"
    LOG_FLAGS += -DTEST_WRITE_SPEED_FEAT
endif

ifeq ($(LOGGING_TO_ONLY_FILE_S),1)
	CXXFLAGS += -DLOG_TO_FILE
endif

ifeq ($(TRACKER_LATENCY_S),1)
	CXXFLAGS += -DLATENCY
endif

ifeq ($(SINGLE_PAXOS_S),1)
	CXXFLAGS += -DSINGLE_PAXOS
endif

ifeq ($(NO_ADD_LOG_TO_NC_S),1)
	CXXFLAGS += -DNO_ADD_LOG_TO_NC
endif

ifeq ($(DISABLE_REPLAY_S),1)
	CXXFLAGS += -DDISABLE_REPLAY
endif

ifeq ($(SKEWED_WORKLOAD_S),1)
	CXXFLAGS += -DSKEWED
endif


ifeq ($(REPLAY_FOLLOWER_S),1)
	CXXFLAGS += -DALLOW_FOLLOWER_REPLAY
endif

ifeq ($(PAXOS_INTERCEPT_S),1)
	CXXFLAGS += -DALLOW_PAXOS_INTERCEPT
endif

ifeq ($(CHECK_INVARIANTS_S),1)
	CXXFLAGS += -DCHECK_INVARIANTS
endif
ifeq ($(EVENT_COUNTERS_S),1)
	CXXFLAGS += -DENABLE_EVENT_COUNTERS
endif
CXXFLAGS += -include masstree/config.h
OBJDEP += masstree/config.h
O := $(O).masstree
CXXFLAGS += -DREAD_MY_WRITES=$(STO_RMW_S)
CXXFLAGS += -DHASHTABLE=$(HASHTABLE_S)
TOP     := $(shell echo $${PWD-`pwd`})
LDFLAGS := -lpthread -lnuma -lrt -lprofiler -lrpc
ifeq ($(GPROF_S),1)
        LDFLAGS += -pg -static-libstdc++ -static-libgcc 
endif

LZ4LDFLAGS := -Lthird-party/lz4 -llz4 -Wl,-rpath,$(TOP)/third-party/lz4

ifeq ($(USE_MALLOC_MODE_S),1)
        CXXFLAGS+=-DUSE_JEMALLOC
        LDFLAGS+=-ljemalloc
	MASSTREE_CONFIG+=--with-malloc=jemalloc
else ifeq ($(USE_MALLOC_MODE_S),2)
        CXXFLAGS+=-DUSE_TCMALLOC
        LDFLAGS+=-ltcmalloc
	MASSTREE_CONFIG+=--with-malloc=tcmalloc
else ifeq ($(USE_MALLOC_MODE_S),3)
        CXXFLAGS+=-DUSE_FLOW
        LDFLAGS+=-lflow
	MASSTREE_CONFIG+=--with-malloc=flow
else
	MASSTREE_CONFIG+=--with-malloc=malloc
endif

ifneq ($(strip $(CUSTOM_LDPATH)), )
        LDFLAGS+=$(CUSTOM_LDPATH)
endif

SRCFILES = allocator.cc \
	btree.cc \
	core.cc \
	counter.cc \
	memory.cc \
	rcu.cc \
	stats_server.cc \
	thread.cc \
	ticker.cc \
	tuple.cc \
	txn_btree.cc \
	txn.cc \
	txn_proto2_impl.cc \
	varint.cc

MASSTREE_SRCFILES = masstree/compiler.cc \
	masstree/str.cc \
	masstree/string.cc \
	masstree/straccum.cc \
	masstree/json.cc \
	masstree/kvthread.cc

OBJFILES := $(patsubst %.cc, $(O)/%.o, $(SRCFILES))

MASSTREE_OBJFILES := $(patsubst masstree/%.cc, $(O)/%.o, $(MASSTREE_SRCFILES))

BENCH_CXXFLAGS := $(CXXFLAGS)
BENCH_LDFLAGS := $(LDFLAGS) -lz -lrt -lcrypt -laio -ldl -lssl -lcrypto

BENCH_SRCFILES = benchmarks/bench.cc \
	benchmarks/encstress.cc \
	benchmarks/bid.cc \
	benchmarks/queue.cc \
	benchmarks/tpcc.cc \
	benchmarks/tpcc_simple.cc\
	benchmarks/ycsb.cc \
	benchmarks/micro_bench.cc \
	benchmarks/sto/ThreadPool.cc \
	benchmarks/sto/function_pool.cc \
	benchmarks/sto/OutputDataSerializer.cpp \
	benchmarks/sto/SerializeUtility.cpp \
	benchmarks/sto/ReplayDB.cc \
	benchmarks/sto/Transaction.cc \
	benchmarks/sto/MassTrans.cc \
	benchmarks/sto/TRcu.cc \
	benchmarks/sto/Packer.cc

BENCH_OBJFILES := $(patsubst %.cc, $(O)/%.o, $(BENCH_SRCFILES))

all: $(O)/test

$(O)/benchmarks/%.o: benchmarks/%.cc $(O)/buildstamp $(O)/buildstamp.bench $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) -Imasstree $(BENCH_CXXFLAGS) -c $< -o $@

$(O)/benchmarks/sto/%.o: benchmarks/sto/%.cc $(O)/buildstamp $(O)/buildstamp.bench $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) -Imasstree $(BENCH_CXXFLAGS) -c $< -o $@

.PHONY: dbclient
dbclient: $(O)/benchmarks/dbclient

$(O)/benchmarks/dbclient: $(O)/benchmarks/dbclient.o $(OBJFILES) $(MASSTREE_OBJFILES) $(BENCH_OBJFILES) third-party/paxos/build/libtxlog.so third-party/lz4/liblz4.so
	$(CXX) -o $(O)/benchmarks/dbclient $^ $(BENCH_LDFLAGS) $(LZ4LDFLAGS) -Lthird-party/paxos/build $(LOG_FLAGS)

.PHONY = ht_mt2
ht_mt2:  $(O)/benchmarks/ht_mt2

$(O)/benchmarks/ht_mt2: $(O)/benchmarks/sto/ht_mt2.o $(OBJFILES) $(MASSTREE_OBJFILES) $(BENCH_OBJFILES) third-party/paxos/build/libtxlog.so third-party/lz4/liblz4.so
	$(CXX) -pthread -o $(O)/benchmarks/ht_mt2 $^ $(BENCH_LDFLAGS) $(LZ4LDFLAGS) -Lthird-party/paxos/build $(LOG_FLAGS)

$(O)/%.o: %.cc $(O)/buildstamp $(OBJDEP)
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MASSTREE_OBJFILES) : $(O)/%.o: masstree/%.cc masstree/config.h
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

third-party/lz4/liblz4.so:
	make -C third-party/lz4 library

.PHONY: test
test: $(O)/test

$(O)/test: $(O)/test.o $(OBJFILES) $(MASSTREE_OBJFILES) third-party/lz4/liblz4.so
	$(CXX) -o $(O)/test $^ $(LDFLAGS) $(LZ4LDFLAGS)

.PHONY: persist_test
persist_test: $(O)/persist_test

$(O)/persist_test: $(O)/persist_test.o third-party/lz4/liblz4.so
	$(CXX) -o $(O)/persist_test $(O)/persist_test.o $(LDFLAGS) $(LZ4LDFLAGS)

.PHONY: stats_client
stats_client: $(O)/stats_client

$(O)/stats_client: $(O)/stats_client.o
	$(CXX) -o $(O)/stats_client $(O)/stats_client.o $(LDFLAGS)

masstree/config.h: $(O)/buildstamp.masstree masstree/configure masstree/config.h.in
	rm -f $@
	cd masstree; ./configure $(MASSTREE_CONFIG)
	if test -f $@; then touch $@; fi

masstree/configure masstree/config.h.in: masstree/configure.ac
	cd masstree && autoreconf -i && touch configure config.h.in

.PHONY: dbtest
dbtest: $(O)/benchmarks/dbtest

$(O)/benchmarks/dbtest: $(O)/benchmarks/dbtest.o $(OBJFILES) $(MASSTREE_OBJFILES) $(BENCH_OBJFILES) third-party/paxos/build/libtxlog.so third-party/lz4/liblz4.so
	$(CXX) -o $(O)/benchmarks/dbtest $^ $(BENCH_LDFLAGS) $(LZ4LDFLAGS) -Lthird-party/paxos/build $(LOG_FLAGS)

DEPFILES := $(wildcard $(O)/*.d $(O)/*/*.d $(O)/*/*/*.d masstree/_masstree_config.d)
ifneq ($(DEPFILES),)
-include $(DEPFILES)
endif

ifeq ($(wildcard masstree/GNUmakefile.in),)
INSTALL_MASSTREE := $(shell git submodule init; git submodule update)
endif

# UPDATE_MASSTREE := $(shell cd ./`git rev-parse --show-cdup` && cur=`git submodule status --cached masstree | head -c 41 | tail -c +2` && if test -z `cd masstree; git rev-list -n1 $$cur^..HEAD 2>/dev/null`; then (echo Updating masstree... 1>&2; cd masstree; git checkout -f master >/dev/null; git pull; cd ..; git submodule update masstree); fi)

ifneq ($(strip $(DEBUG_S).$(CHECK_INVARIANTS_S).$(EVENT_COUNTERS_S)),$(strip $(DEP_MAIN_CONFIG)))
DEP_MAIN_CONFIG := $(shell mkdir -p $(O); echo >$(O)/buildstamp; echo "DEP_MAIN_CONFIG:=$(DEBUG_S).$(CHECK_INVARIANTS_S).$(EVENT_COUNTERS_S)" >$(O)/_main_config.d)
endif

ifneq ($(strip $(MASSTREE_CONFIG)),$(strip $(DEP_MASSTREE_CONFIG)))
DEP_MASSTREE_CONFIG := $(shell mkdir -p $(O); echo >$(O)/buildstamp.masstree; echo DEP_MASSTREE_CONFIG:='$(MASSTREE_CONFIG)' >masstree/_masstree_config.d)
endif

$(O)/buildstamp $(O)/buildstamp.bench $(O)/buildstamp.masstree:
	@mkdir -p $(@D)
	@echo >$@

.PHONY: clean
clean:
	rm -rf out-*
	make -C third-party/lz4 clean

.PHONY: write_clean
write_clean:
	rm -rf out-*
	mkdir -p $(REPLAY_LOG_FOLDER)
	cp -r $(LOG_FOLDER)/* $(REPLAY_LOG_FOLDER)
	rm -rf $(LOG_FOLDER)/*
	make -C third-party/lz4 clean

.PHONY: replay_clean
replay_clean:
	rm -rf out-*
	rm -rf $(REPLAY_LOG_FOLDER)/*
	make -C third-party/lz4 clean

.PHONY: paxos
paxos:
	cd ./third-party/paxos && make

.PHONY: paxos_async_commit_test
paxos_async_commit_test: $(O)/benchmarks/paxos_async_commit_test

$(O)/benchmarks/paxos_async_commit_test: $(O)/benchmarks/paxos_async_commit_test.o $(OBJFILES) $(MASSTREE_OBJFILES) $(BENCH_OBJFILES) third-party/paxos/build/libtxlog.so third-party/lz4/liblz4.so
	$(CXX) -o $(O)/benchmarks/paxos_async_commit_test $^ $(BENCH_LDFLAGS) $(LZ4LDFLAGS) -Lthird-party/paxos/build $(LOG_FLAGS)
