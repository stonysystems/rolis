import glob
import json
import os, sys
from collections import Counter
from pprint import pprint

from tqdm import tqdm

CONST_LOG_PATH = "/home/jay/prev_logs/replay_logs/1/"
CONST_PERF_PATH = "/home/jay/run_perf/"


class Report:
    def __init__(self, dict_obj):
        self._dict = dict_obj

    def __len__(self):
        return 0

    def __repr__(self):
        result = json.dumps(self._dict)
        return result

    def length(self):
        return len(self._dict)


class CollateObject:
    def __init__(self):
        self.dict_ = dict()
        pass

    def __set__(self, instance, value):
        pass

    def __get__(self, instance, owner):
        pass

    def update(self, other):
        for k, v in other.items():
            old = self.dict_.get(k, 0)
            old += v
            self.dict_[k] = old

    def items(self):
        return self.dict_.items()

    def report(self):
        return Report(self.dict_)


def read_file_give_me_count_array(file_full_path):
    aggr_dict = {}
    with open(file_full_path) as fs:
        lines = fs.readlines()
        for each_line in lines:
            cntr = Counter(each_line.split(":"))
            aggr_dict.update(cntr)
    return aggr_dict


def get_all_threads_registered(parent_path):
    assert os.path.exists(parent_path)
    dir_list = os.listdir(parent_path)
    assert len(dir_list) != 0
    return dir_list


def read_csv_files(files_full_path_as_list, collated_cls_obj):
    for each in tqdm(files_full_path_as_list):
        obj_ = read_file_give_me_count_array(each)
        collated_cls_obj.update(obj_)
    print("Finished %s" % sys._getframe().f_code.co_name)


def _test_read_file_give_me_count_array():
    file_full_path = CONST_LOG_PATH + "/Log-ThreadID:647.txt"
    ret_dict = read_file_give_me_count_array(file_full_path)
    assert len(ret_dict) != 0


def _test_get_all_threads_registered():
    parent_path = CONST_LOG_PATH
    threads_as_list = get_all_threads_registered(parent_path)
    assert len(threads_as_list) == 2


def _test_read_csv_files():
    parent_path = CONST_LOG_PATH
    files_ = ["Log-ThreadID:51.txt", "Log-ThreadID:52.txt", "Log-ThreadID:53.txt"]
    log_folder = "10"

    files_full_path_as_list = [os.sep.join([parent_path, log_folder, "csv_logs", i]) for i in files_]
    aggregator = CollateObject()
    read_csv_files(files_full_path_as_list, aggregator)
    assert aggregator.report().length() != 0


def get_all_file_in_path(p_path):
    ans = []
    for root, subdirs, files in os.walk(p_path):
        for file in os.listdir(root):
            filePath = os.path.join(root, file)
            if os.path.isfile(filePath):
                ans.append(filePath)
    return ans


if __name__ == '__main__':

    aggregator = CollateObject()
    try:
        parent_path = sys.argv[1]
    except:
        parent_path = CONST_LOG_PATH

    try:
        RUN_ID = sys.argv[2]
    except:
        RUN_ID = "0"
    read_status_case = CONST_LOG_PATH + RUN_ID
    files_full_path_as_list = []

    files_full_path_as_list = get_all_file_in_path(parent_path)
    read_csv_files(files_full_path_as_list, aggregator)

    aggregator = {i: v for i, v in aggregator.items() if i != ''}
    sort_by_time = sorted(aggregator.items(), key=lambda x: x[0])
    import numpy as np

    if not os.path.exists(read_status_case):
        os.makedirs(read_status_case)
    print("Average for case {} is {}".format(RUN_ID,sum(list(map(lambda x:x[1],sort_by_time)))/len(aggregator)))
    np.save("%s/file2.npy" % read_status_case, np.array(sort_by_time))
    print(sort_by_time)
    exit(0)
