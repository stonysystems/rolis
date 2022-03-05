import argparse
import os
import sys
from tqdm import tqdm
from subprocess import Popen, PIPE, STDOUT
import copy
from pprint import pprint
import config
import re
import numpy as np
from itertools import chain
import pandas as pd


def read_replay_perf_one_file(file_name, report_dict_p):
    with open(file_name) as fs:
        all_lines = fs.readlines()
        colon_splitted = all_lines[0].split(':')
        for each in colon_splitted:
            old = report_dict_p.get(each, 0)
            old += 1
            report_dict_p[each] = old


def extract_replay_perf():
    report_dict = {}
    parent_path = config.BASE.CONST_REPLAY_PERF_PATH
    if os.path.exists(parent_path) is False:
        return 0
    for each in os.listdir(parent_path):
        if 'txt' in each:
            read_replay_perf_one_file(parent_path + each, report_dict)
    if '' in report_dict:
        del report_dict['']
    vals = report_dict.values()
    cnt = len(vals)
    sum_ = sum(vals)
    if cnt > 1:
        return sum_ / float(cnt)
    else:
        raise Exception("No data to process in dir {}".format(parent_path))


def build_run(RUN_COMMAND_PASSED):
    print("[BuildRun:RUN_COMMAND_PASSED] run command=   \"{0}\"".format(RUN_COMMAND_PASSED))
    p = Popen(RUN_COMMAND_PASSED, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)
    # with open(config.BASE.CONST_RESULT_SAVER_PATH, "a+") as fs:
    #     op = p.stdout.read()
    #     fs.write(op)
    p.stdout.read()


def master_run(RUN_COMMAND_PASSED, show_op=False):
    print("[AppRun:RUN_COMMAND_PASSED] run command=   \"{0}\"".format(RUN_COMMAND_PASSED))
    p = Popen(RUN_COMMAND_PASSED, shell=True, stdin=PIPE, stdout=PIPE, stderr=STDOUT, close_fds=True)
    # with open(config.BASE.CONST_RESULT_SAVER_PATH, "a+") as fs:
    #     op = p.stdout.read()
    #     fs.write(op)
    if show_op:
        xxx = ""
        for line in iter(p.stdout.readline, ''):
            xxx += line
            print(line)
    else:
        xxx = str(p.stdout.read().decode("utf-8")).rstrip()
    return xxx


class BuildRun:
    def run(self, command):
        build_run(command)
        print("[BuildRun] run command=   \"{0}\"".format(command))

    def build_run(self, command):
        build_run(command)
        print("[BuildRun] run command=   \"{0}\"".format(command))

    def clean(self):
        self.run(config.BASE.CONST_CLEAN)


class AppRun:
    def app_run(self, command, show_op=False):
        fake_reports_str = master_run(command, show_op)
        # include fake reports to collect the statistics
        # fake_reports_str = """
        #         PinToWarehouseId(): coreid=5 pinned to whse=1 (partid=0)
        # PinToWarehouseId(): coreid=8 pinned to whse=2 (partid=1)
        # PinToWarehouseId(): coreid=1 pinned to whse=2 (partid=1)
        # PinToWarehouseId(): coreid=4 pinned to whse=1 (partid=0)
        # PinToWarehouseId(): coreid=7 pinned to whse=1 (partid=0)
        # cpu1 starting faulting region (536870912 bytes / 256 hugepgs)
        # num batches: 30
        # PinToWarehouseId(): coreid=6 pinned to whse=2 (partid=1)
        # PinToWarehouseId(): coreid=2 pinned to whse=1 (partid=0)
        # cpu0 starting faulting region (536870912 bytes / 256 hugepgs)
        # [INFO] finished loading warehouse
        # [INFO]   * average warehouse record length: 97 bytes
        # [INFO] finished loading item
        # [INFO]   * average item record length: 84.1739 bytes
        # cpu1 finished faulting region in 194.64 ms
        # cpu0 finished faulting region in 196.312 ms
        # PinToWarehouseId(): coreid=2 pinned to whse=2 (partid=1)
        # [INFO] finished loading district
        # [INFO]   * average district record length: 99 bytes
        # [INFO] finished loading order (w=1)
        # [INFO] finished loading order (w=2)
        # [INFO] finished loading customer (w=2)
        # [INFO] finished loading customer (w=1)
        # [INFO] finished loading stock (w=2)
        # [INFO] finished loading stock (w=1)
        # timed region dataloading took 950.46 ms
        # DB size: 1336.44 MB
        # [0, 0, 0] txns persisted in loading phase
        # table customer_0 size 0
        # table customer_name_idx_0 size 0
        # table district_0 size 0
        # table history_0 size 0
        # table item_0 size 0
        # table new_order_0 size 0
        # table oorder_0 size 0
        # table oorder_c_id_idx_0 size 0
        # table order_line_0 size 0
        # table stock_0 size 0
        # table stock_data_0 size 0
        # table warehouse_0 size 0
        # starting benchmark...
        # tpcc: worker id 64 => warehouses [1, 2)
        # tpcc: worker id 65 => warehouses [2, 3)
        # --------------------
        # Table Name= customer ID= 10001
        # Table Name= customer_name_idx ID= 10002
        # Table Name= district ID= 10003
        # Table Name= history ID= 10004
        # Table Name= item ID= 10005
        # Table Name= new_order ID= 10006
        # Table Name= oorder ID= 10007
        # Table Name= oorder_c_id_idx ID= 10008
        # Table Name= order_line ID= 10009
        # Table Name= stock ID= 10010
        # Table Name= stock_data ID= 10011
        # Table Name= warehouse ID= 10012
        # mt_get: 19746734, mt_put: 6390496, mt_del: 55329, mt_scan: 803772, mt_rscan: 39634, ht_get: 0, ht_put: 0, ht_insert: 0, ht_del: 0
        # --------------------
        # --- table statistics ---
        # table customer_0 size 0 (+0 records)
        # table customer_name_idx_0 size 0 (+0 records)
        # table district_0 size 0 (+0 records)
        # table history_0 size 0 (+0 records)
        # table item_0 size 0 (+0 records)
        # table new_order_0 size 0 (+0 records)
        # table oorder_0 size 0 (+0 records)
        # table oorder_c_id_idx_0 size 0 (+0 records)
        # table order_line_0 size 0 (+0 records)
        # table stock_0 size 0 (+0 records)
        # table stock_data_0 size 0 (+0 records)
        # table warehouse_0 size 0 (+0 records)
        # --- benchmark statistics ---
        # runtime: 10.0007 sec
        # memory delta: 2273.9 MB
        # memory delta rate: 227.373 MB/sec
        # logical memory delta: 62.7251 MB
        # logical memory delta rate: 6.27204 MB/sec
        # agg_nosync_throughput: 98810.3 ops/sec
        # avg_nosync_per_core_throughput: 49405.2 ops/sec/core
        # agg_throughput: 98808 ops/sec
        # avg_per_core_throughput: 49404 ops/sec/core
        # agg_persist_throughput: 98808 ops/sec
        # avg_per_core_persist_throughput: 49404 ops/sec/core
        # avg_latency: 0.0201908 ms
        # avg_persist_latency: 0 ms
        # agg_abort_rate: 10.1992 aborts/sec
        # avg_per_core_abort_rate: 5.09962 aborts/sec/core
        # txn breakdown: [[Delivery, 39129], [NewOrder, 445352], [OrderStatus, 39634], [Payment, 424423], [StockLevel, 39717]]
        # --- system counters (for benchmark) ---
        # --- perf counters (if enabled, for benchmark) ---
        # --- allocator stats ---
        # [allocator] ncpus=2
        # [allocator] cpu=0 fully_faulted?=1 remaining=536870912 bytes
        # [allocator] cpu=1 fully_faulted?=1 remaining=536870912 bytes
        # ---------------------------------------
        # dumping heap profile...
        # printing jemalloc stats...
        # 98807.958569 98807.958569 0.020191 0.000000 10.199242 445352
        # Total Delete Observed 55300
        #         """
        #table_meta_dict = extract_table_operations_statistics(fake_reports_str)
        jemalloc_dict = extract_jemalloc_stats(fake_reports_str)
        print("[AppRun] run command=   \"{0}\"".format(command))
        return jemalloc_dict


class ReplayApp:
    def get_read_perf(self):
        avg_throughput = extract_replay_perf()
        return {"average_read_throughput": avg_throughput}

    def replay_run(self, command, show_op=False):
        fake_reports_str = master_run(command, show_op)
        print("[AppRun] run command=   \"{0}\"".format(command))
        # include fake reports to collect the statistics
        # fake_reports_str = """
        #                             Time Taken to Load the files :2.982477"
        #                             Time Taken to Process : 4.251349
        #                             Total Transactions made : 185264012
        #                             Replay Throughput : 2.58992e+06
        #                             Total success : 42201081
        #                             Total Processed Log size in (MB) : 1932.62
        #                             Observed delete count : 56430
        #                        """
        r_stats = extract_replay_stats(fake_reports_str)
        read_perf = self.get_read_perf()
        return {"replay_run_statistics": r_stats, "read_perf": read_perf}


class BuildError(Exception):
    def __repr__(self):
        return self.__name__


class BuildProcess(BuildError, BuildRun):
    def build(self, cmd):
        self.build_run(cmd)

    def clean(self, cmd=config.BASE.CONST_CLEAN):
        self.build_run(cmd)


class DBTEST(BuildProcess, BuildError, AppRun):
    def run_db_test(self, cmd, show_op=False):
        db_test_results = self.app_run(cmd, show_op)
        return db_test_results

    def db_test_clean(self, cmd):
        BuildProcess().run(cmd)


class ReplayRunProcess(BuildProcess, ReplayApp):
    PLAYER = config.REPLAY()

    def replay_build(self, **kwargs):
        cmd = self.PLAYER.CONST_BUILD_COMMAND
        plot_replay_speed = kwargs.get('plot_replay_speed', 1)  # PLOT_REPLAY_SPEED
        cmd = cmd.format(PLOT_REPLAY_SPEED=plot_replay_speed)
        BuildProcess().build(cmd=cmd)

    def run_replay(self, **kwargs):
        cmd = self.PLAYER.build_run_command_prepare(**kwargs)
        do_show_op = kwargs.get('do_show_op', False)
        replay_stats = self.replay_run(cmd, show_op=do_show_op)
        return replay_stats

    def replay_clean(self):
        cmd = self.PLAYER.CONST_CLEAN
        BuildProcess().clean(cmd=cmd)


def extract_table_operations_statistics(given_string_passed):
    mutable_dict_passed = {"mt_get": 0, "mt_put": 0, "mt_del": 0, "mt_scan": 0, "mt_rscan": 0,
                           "ht_get": 0, "ht_put": 0, "ht_insert": 0, "ht_del": 0}
    filtered = list(filter(lambda x: 'mt_get:' in x, given_string_passed.split("\n")))
    if len(filtered) != 1:
        raise Exception("No meta information avaliable for this test")
    given_string_passed = filtered[0]
    comma_sep_values = given_string_passed.split(",")
    for each in comma_sep_values:
        semi_columns_sep_values = each.split(":")
        key = semi_columns_sep_values[0].strip()
        val = float(semi_columns_sep_values[1].strip())
        mutable_dict_passed[key] = val
    return mutable_dict_passed


def extract_jemalloc_stats(given_string_passed):
    keys = "agg_throughput	avg_per_core_throughput	avg_latency	avg_persist_latency	agg_abort_rate	NewOrder".split()
    immutable_dict_passed = {k: 0 for k in keys}
    #str(xxx.decode("utf-8")).split("\n")
    splitted = given_string_passed.split("\n")
    splitted = [i.strip() for i in splitted]
    idx = 0
    try:
        idx = splitted.index('printing jemalloc stats...') + 1
    except:
        raise Exception("jemalloc stats not found correctly here")
    given_string_passed = splitted[idx]
    sep_values = given_string_passed.split(" ")
    for key, val in zip(keys, sep_values):
        immutable_dict_passed[key] = float(val.strip())
    return immutable_dict_passed


def _test_jemalloc_stats():
    given_string = "101396.868595 101396.868595 0.019666 0.000000 21.699201 457173"
    immutable_dict = extract_jemalloc_stats(given_string)
    print(immutable_dict)


def _test_table_stats():
    given_table_meta_string = "mt_get: 19924283, mt_put: 6449108, mt_del: 55693, mt_scan: 810909, mt_rscan: 39995, ht_get: 0, " \
                              "ht_put: 0, ht_insert: 0, ht_del: 0 "
    table_meta_dict = extract_table_operations_statistics(given_table_meta_string)
    print(table_meta_dict)


def one_run_statistics(test_name, result):
    for thread_id in [2, 3, 4]:
        given_string = "101396.868595 101396.868595 0.019666 0.000000 21.699201 457173"
        given_string = given_string + str(thread_id)
        perf_dict = extract_jemalloc_stats(given_string)
        given_table_meta_string = "mt_get: 19924283, mt_put: 6449108, mt_del: 55693, mt_scan: 810909, mt_rscan: 39995, ht_get: 0, " \
                                  "ht_put: 0, ht_insert: 0, ht_del: 0 "
        table_meta_dict = extract_table_operations_statistics(given_table_meta_string)
        result.append({"test_name": test_name,
                       "thread_id": thread_id,
                       "table_stats": table_meta_dict,
                       "perf_stats": perf_dict})


def get_test_parameter(test_class, test_name_, test_consts):
    # fix build command
    serialize = test_name_.get("serialize", 0)
    is_write_speed = test_name_.get("plot_write_speed", 0)
    run_time = test_name_.get("run_time", 0)
    sto_batching_config = test_name_.get("sto_batching_config", 0)
    replay_follower = test_name_.get("replay_follower", 0)


    test_class.build_build_command_prepare(serialize=serialize,
                                           plot_write_speed=is_write_speed,
                                           sto_batching_config=sto_batching_config,
                                           replay_follower=replay_follower)

    # fix some runnable command
    test_class.build_run_command_prepare(run_time=run_time)

    # get all of filled parameters, all of CONST prefix are important
    parameters = test_class.get_all_params()
    # return parameters
    return parameters


class RunSerialize:
    serialized_tests = {"case a": config.SEARIALIZE_A(),
                        "case b": config.SEARIALIZE_B()}

    @staticmethod
    def getTestParameters(test_name_passed, constants, kwargs, serialized_test_case="case a"):
        cls_obj = RunSerialize.serialized_tests.get(serialized_test_case, config.SEARIALIZE_A())
        cls_obj.update_scale_threads(start=kwargs.thread_count_start,
                                     end=kwargs.thread_count_end,
                                     scale=kwargs.db_scale)
        params = get_test_parameter(cls_obj, test_name_passed, constants)
        return params


def build_test_case(test_case_):
    """
    :param test_case_:
    :return:
    """
    run_class = DBTEST()
    replay_class = ReplayRunProcess()
    test_case_c = test_case_.copy()

    test_case_c["run_obj"] = run_class
    test_case_c["replay_obj"] = replay_class
    return test_case_c


def count_log_files_in_dir(directory_with_full_path):
    print('*' * 20)
    try:
        files = os.listdir(directory_with_full_path)
    except:
        files = []
    if not len(files):
        return 0
        # raise Exception("No files found in dir={} for replay process")
    ans = []
    for each in files:
        if os.path.isfile(directory_with_full_path + each) and config.BASE.REPLAY_LOG_FILE_EXT in each:
            ans.append(directory_with_full_path + each)
    print("File Count Observed", len(ans))
    print('*' * 20)
    return len(ans)


def do_filter_replay_util(signature, input_vals):
    filtered = list(filter(lambda x: signature in x, input_vals))
    if len(filtered) != 1:
        print(input_vals, filtered)
        assert len(filtered) != 1
    ans = re.findall(r'[\d\.\d]+', filtered[0])
    if len(ans) != 1:
        print("Filtered ans=", ans)
        assert len(ans) != 1
    value = float(ans[0])
    return value


def extract_replay_load_time(splitted):
    signature = "Time Taken to Load the files"
    return do_filter_replay_util(signature, splitted)


def extract_total_txn_made(splitted):
    signature = "Total Transactions made"
    return do_filter_replay_util(signature, splitted)


def extract_replay_process_time(splitted):
    signature = "Time Taken to Process"
    return do_filter_replay_util(signature, splitted)


def extract_success_processed_entries(splitted):
    signature = "Total success"
    return do_filter_replay_util(signature, splitted)


def extract_processed_log_size_in_MB(splitted):
    signature = "Total Processed Log size in"
    return do_filter_replay_util(signature, splitted)


def extract_delete_ops_performed(splitted):
    signature = "Observed delete count"
    return do_filter_replay_util(signature, splitted)


def _test_replay_fakes():
    fake_reports_str = """
                                        Time Taken to Load the files :2.982477"
                                        Time Taken to Process : 4.251349
                                        Total success : 42201081
                                        Total Processed Log size in (MB) : 1932.62
                                        Observed delete count : 56430
                                   """
    ans = extract_replay_stats(fake_reports_str)
    pprint(ans)
    return ans


def extract_replay_stats(fake_reports_str):
    splitted = fake_reports_str.split("\n")
    read_load_time = extract_replay_load_time(splitted)
    toal_txns_made = extract_total_txn_made(splitted)
    read_process_times = extract_replay_process_time(splitted)
    success_processed_entries = extract_success_processed_entries(splitted)
    processed_log_size_in_MB = extract_processed_log_size_in_MB(splitted)
    delete_ops_performed = extract_delete_ops_performed(splitted)
    return {"read_load_time": read_load_time,
            "read_process_times": read_process_times,
            "total_txns" : toal_txns_made,
            "replay_throughput" : float(toal_txns_made)/float(read_load_time+read_process_times),
            "success_processed_entries": success_processed_entries,
            "processed_log_size_in_MB": processed_log_size_in_MB,
            "delete_ops_performed": delete_ops_performed}


def _test_one_run_statistics(kwargs):
    test_names = {
        # {"test_name":"STO Org"},
        # by serialize we mean paxos enabled
        "only-silo": [{"test_name": "silo-paxos-disabled", "serialize": 0, "plot_write_speed": 0,
                                "run_time": kwargs.run_time,
                                "sto_batching_config": 0,
                                "replay_follower": 0,
                                "plot_replay_speed": 0}],
        "silo-paxos-enabled": [{"test_name": "silo-paxos-enabled", "serialize": 1, "plot_write_speed": 0,
                                "run_time": kwargs.run_time,
                                "sto_batching_config": kwargs.sto_batching_config,
                                "replay_follower": 0,
                                "plot_replay_speed": 1}],

        "silo-paxos-replay-enabled": [{"test_name": "silo-paxos-replay-enabled", "serialize": 1, "plot_write_speed": 0,
                               "run_time": kwargs.run_time,
                               "sto_batching_config": kwargs.sto_batching_config,
                               "replay_follower": 1,
                               "plot_replay_speed": 1}],

    }
    if kwargs.serialize is False:
        del test_names["silo-paxos-enabled"]
        del test_names["silo-paxos-replay-enabled"]
    keys_to_select = ["only-silo"] if not kwargs.serialize else ["silo-paxos-enabled", "silo-paxos-replay-enabled"]
    if kwargs.run_all:
        test_cases = test_names.values()
        test_cases = list(chain.from_iterable(test_cases))
        test_names = copy.deepcopy(test_cases)
    else:
        test_cases = [test_names[x] for x in keys_to_select]#test_names[keys_to_select].values()
        test_cases = list(chain.from_iterable(test_cases))
        test_names = copy.deepcopy(test_cases)

    do_show_op = kwargs.show_app_output
    serialize_obj = RunSerialize()

    xxx = {}
    test_cases_to_run = []
    # build test case
    report_collection = {}
    global_row = []
    for test_name_ in test_names:
        test_to_run = serialize_obj.getTestParameters(test_name_passed=test_name_, constants=xxx, kwargs=kwargs)
        test_cases_to_run.append((test_name_["test_name"],
                                  build_test_case(test_to_run.copy()), test_name_["serialize"],
                                  test_name_["plot_write_speed"], test_name_["plot_replay_speed"], test_name_["sto_batching_config"], test_name_["replay_follower"]))

    for test_case_name, each_runnable_test_case, do_serialize, do_test_wr, do_test_replay, do_test_sto_batching, do_test_sto_replay_follower in test_cases_to_run:
        for each_thread, scale in each_runnable_test_case.get("CONST_SCALE_AND_THREADS", []):
            pass
            # <  SERIALIZE >
            # run for every thread count <serialize,replay>
            dbtest = each_runnable_test_case["run_obj"]
            # clean dbtest
            dbtest.clean()
            # build dbtest
            dbtest.build(each_runnable_test_case["CONST_BUILD_COMMAND"])
            # run run_db_test
            run_cmd = each_runnable_test_case["CONST_RUN_COMMAND"]
            run_cmd = run_cmd.format(PAXOS_BATCHING=kwargs.paxos_batching, STO_BATCHING=kwargs.sto_batching, PAXOS_SITE_CONFIG=kwargs.paxos_site_config,
                                     SCALE=scale, THREAD_COUNT=each_thread)
            db_test_results = copy.deepcopy(dbtest.run_db_test(run_cmd, show_op=do_show_op))
            # clean db_test
            dbtest.db_test_clean(each_runnable_test_case["CONST_CLEAN"])
            replay_perf = {}
            # if do_serialize:
            #     file_count = count_log_files_in_dir(each_runnable_test_case["CONST_REPLAY_READER_PATH"])
            #     # < REPLAY >
            #     ht_mt2 = each_runnable_test_case["replay_obj"]
            #     # build
            #     ht_mt2.replay_build()
            #     # run
            #     replay_perf = copy.deepcopy(ht_mt2.run_replay(file_count=file_count,
            #                                                   thread_count=kwargs.num_of_replay_threads,
            #                                                   do_show_op=do_show_op))
            #     # replay_clean
            #     ht_mt2.replay_clean()
            # else:
            #     pass
            demo_dict = {}
            demo_dict["test_name"] = test_case_name
            demo_dict["nThreads"] = each_thread
            demo_dict["scale"] = scale
            demo_dict.update(db_test_results)
            global_row.append(demo_dict)
            
            report_collection[(test_case_name, each_thread, scale)] = (each_thread, scale,
                                                                       {"dbtest_results": db_test_results})
            print("=============================================")
    global_row = pd.DataFrame(data=global_row)
    pprint(report_collection)
    ppath = config.BASE.CONST_RESULT_SAVER_PATH
    tempstr = "SBC_" + str(kwargs.sto_batching_config) + "_SB_" + str(kwargs.sto_batching) + "_PB_" + str(kwargs.paxos_batching) +"_"+ kwargs.output_file_suffix
    ppath = ppath.format(suffix=tempstr)
    try:
        global_row.to_csv(ppath, index=False)
        print("CSV {} has been generated successfully".format(ppath))
        print("=============================================")
    except:
        print("Error in saving the csv, ", ppath)
        pass


def save_this_result():
    op_dict = {"mt_get": 0, "mt_put": 0, "mt_del": 0, "mt_scan": 0, "mt_rscan": 0,
               "ht_get": 0, "ht_put": 0, "ht_insert": 0, "ht_del": 0}
    results = {"thread_count": 0,
               "scale": 0,
               "throughput": 0,
               "abort_rate": 0,
               "ops": op_dict}
    pass


if __name__ == "__main__":
    """
    python dbtest_build_and_save.py --show_app_output=False --num_of_replay_threads=1 --run_time=10 --thread_count_start=1 --thread_count_end=2 --serialize=True --output_file_suffix="s1to2"
    """
    parser = argparse.ArgumentParser(description='Throughput report')
    parser.add_argument('--show_app_output', type=bool, help='Show output in run.log', default=False)
    #parser.add_argument('--num_of_replay_threads', type=int, help='Output dir for image', default=config.BASE.REPLAY_CONST_THREAD_COUNT)
    parser.add_argument('--run_all', type=bool, help='run for all test cases(serialize, non serialize)', default=False)
    parser.add_argument('--run_time', type=int, help='Output dir for image', default=30)
    parser.add_argument('--db_scale', type=int, help='DB TEST Scale factor', default=1)
    parser.add_argument('--thread_count_start', type=int, help='thread start loop', default=1)
    parser.add_argument('--thread_count_end', type=int, help='thread end loop', default=2)
    parser.add_argument('--serialize', type=int, help='Set to run serialize test case', default=0)
    #parser.add_argument('--replay_follower', type=int, help='replay logs on followers for replication', default=0)
    parser.add_argument('--numa_memory', type=int, help='numa memory enabling', default=4)
    parser.add_argument('--sto_batching_config', type=int, help='Set batching configuration', default=0)
    parser.add_argument('--sto_batching', type=int, help='Set batching for sto', default=1)
    parser.add_argument('--paxos_batching', type=int, help='Set batching for paxos', default=1)
    parser.add_argument('--paxos_site_config', type=str, help='site configuration for paxos', default="")
    parser.add_argument('--output_file_suffix', type=str, help='Output dir for image', default="")

    args = parser.parse_args()
    _test_one_run_statistics(args)
