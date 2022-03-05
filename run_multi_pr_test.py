import logging
import multiprocessing
import os
import subprocess
import traceback
from argparse import ArgumentParser

import sys

LOG_LEVEL = logging.INFO
LOG_FILE_LEVEL = logging.DEBUG
logger = logging.getLogger('paxos')

deptran_home = os.path.split(os.path.realpath(__file__))
deptran_home = deptran_home[0]
g_log_dir = deptran_home + "/log"


def create_parser():
    parser = ArgumentParser()

    parser.add_argument("-N", "--name", dest="experiment_name",
                        help="python log file name", default="paxos-microbench")

    parser.add_argument("-f", "--file", dest="config_files",
                        help="read config from FILE",
                        action='append',
                        default=[], metavar="FILE")

    parser.add_argument("-d", "--duration", dest="c_duration",
                        help="benchmark running duration in seconds", default=60,
                        action="store", metavar="TIME", type=int)

    parser.add_argument("-t", "--server-timeout", dest="s_timeout",
                        help="server heart beat timeout in seconds", default=10,
                        action="store", metavar="TIMEOUT", type=int)

    parser.add_argument("-l", "--log-dir", dest="log_dir",
                        help="log file directory", default=g_log_dir,
                        metavar="LOG_DIR")

    parser.add_argument("-P", "--process-num", dest="process_num",
                        help="number of processes. Assume process 'localhost' is for the leaders",
                        default="0", action='store', type=int)

    parser.add_argument("-T", "--total-req", dest="tot_num",
                        help="tot submission numbers for paxos microbench (do not matter if just using submission interfaces)",
                        action="store", default="100", type=int)

    parser.add_argument("-n", "--concurrent-req", dest="concurrent",
                        help="how many outstanding requests for paxos microbench (do not matter if just using submission interfaces)",
                        action="store", default="32", type=int)

    parser.add_argument("--verbose", dest="verbose",
                        help="verbose option within STO",
                        default=False, action="store_true")

    parser.add_argument("--bench", dest="bench",
                        help="bench option within STO",
                        default="tpcc")

    parser.add_argument("--db-type", dest="db_type",
                        help="db-type option within STO",
                        default="mbta")

    parser.add_argument("--scale-factor", dest="scale_factor",
                        help="scale-factor option within STO",
                        action="store", default="2", type=int)

    parser.add_argument("--num-threads", dest="num_threads",
                        help="num-threads option within STO",
                        action="store", default="2", type=int)

    parser.add_argument("--numa-memory", dest="numa_memory",
                        help="numa-memory option within STO",
                        default="1G")

    parser.add_argument("--p-batch-size", dest="p_batch_size",
                        help="paxos batch size",
                        default="10000")

    parser.add_argument("--sto-batch-size", dest="sto_batch_size",
                        help="batch STO size",
                        default="10")

    parser.add_argument("--runtime", dest="runtime",
                        help="runtime option within STO",
                        action="store", default="10", type=int)

    parser.add_argument("--parallel-loading", dest="parallel_loading",
                        help="parallel-loading option within STO",
                        default=False, action="store_true")

    logging.debug(parser)
    return parser


def setup_logging(log_file_path=None):
    root_logger = logging.getLogger('')
    root_logger.setLevel(LOG_LEVEL)
    logger.setLevel(LOG_FILE_LEVEL)

    if log_file_path is not None:
        print("logging to file: %s" % log_file_path)
        fh = logging.FileHandler(log_file_path)
        fh.setLevel(LOG_FILE_LEVEL)
        formatter = logging.Formatter(
            fmt='%(levelname)s: %(asctime)s: %(message)s')
        fh.setFormatter(formatter)
        root_logger.addHandler(fh)

    ch = logging.StreamHandler()
    ch.setLevel(LOG_LEVEL)
    formatter = logging.Formatter(fmt='%(levelname)s: %(message)s')
    ch.setFormatter(formatter)
    root_logger.addHandler(ch)
    logger.debug('logger initialized')


def gen_process_cmd(config, process_name):
    cmd = []
    # cmd.append("cd " + deptran_home + "; ")
    # cmd.append("export LD_LIBRARY_PATH=$(pwd)/third-party/paxos/build; ")
    # cmd.append("mkdir -p " + config.log_dir + "; ")
    if (process_name == "localhost"):
        s = "./out-perf.masstree/benchmarks/dbtest "

        if (config.verbose):
            s += "--verbose "

        s += "--bench " + config.bench + " " \
                                         "--db-type " + config.db_type + " " \
                                                                         "--scale-factor " + str(
            config.scale_factor) + " " \
                                   "--num-threads " + str(config.num_threads) + " " \
                                                                                "--numa-memory " + config.numa_memory + " "

        if (config.parallel_loading):
            s += "--parallel-loading "

        s += "--runtime " + str(config.runtime) + " "
        s += "-S " + str(config.sto_batch_size) + " "
        s += "-A " + str(config.p_batch_size) + " "

        for fn in config.config_files:
            s += "-F " + fn + " "

        s += "1>'" + config.log_dir + "/proc-" + \
             process_name + ".log' "

        cmd.append(s)
        return ' '.join(cmd)

    s = "./third-party/paxos/build/microbench " + \
        "-b " + \
        "-d " + str(config.c_duration) + " "

    for fn in config.config_files:
        s += "-f " + fn + " "

    s += "-P " + process_name + " " + \
         "-t " + str(config.s_timeout) + " " \
                                         "1>'" + config.log_dir + "/proc-" + process_name + ".log' " + \
         "2>'" + config.log_dir + "/proc-" + process_name + ".err' " + \
         "&"

    cmd.append(s)
    return ' '.join(cmd)


def do_work(cmd):
    # print("[MJAY] args=", cmd)
    subprocess.call([cmd], shell=True)


def starter(config_):
    # arg1, arg2, arg3 = args
    # print("Print# =>", args, "\n")
    # work_args = [(config, 2, 3), (config, 5, 6), (config, 8, 9), ]
    p = multiprocessing.Pool(config_.num_threads)

    ret = 0
    config = config_
    try:
        def get_to_run_one_server(address, config, i):
            logger.info("starting %s @ %s", i, address)
            cmd = gen_process_cmd(config, i)
            return cmd

        def run_one_server(cmd):
            logger.debug("running: %s", cmd)
            # os.system(cmd)
            subprocess.call([cmd], shell=True)

        run_ret = get_to_run_one_server('127.0.0.1', config, "localhost")
        run_one_server(run_ret)

        num_list = list(range(1, config.process_num + 1))
        work_args = []
        for i in num_list:
            work_args.append(get_to_run_one_server('127.0.0.1', config, "p" + str(i)))

        p.map(do_work, work_args)

    except Exception:
        logging.error(traceback.format_exc())
        ret = 1
    finally:
        if ret != 0:
            sys.exit(ret)


def main():
    config = create_parser().parse_args()
    log_path = os.path.join(config.log_dir,
                            config.experiment_name + ".log")
    setup_logging(log_path)

    starter(config)


if __name__ == '__main__':
    main()
