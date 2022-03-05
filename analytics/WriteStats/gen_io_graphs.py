import os, sys
import matplotlib.pyplot as plt
import pandas as pd
from tqdm import tqdm
import numpy as np

df_global = {}
df_global_list = []
len_global_list = []


def mk_png_file(onlyname, target_file_name, imp_file_name):
    try:
        df = pd.read_csv(imp_file_name, header=None)
    except pd.errors.EmptyDataError:
        return
    df.columns = ["XXX", "MBps", "spent", "timestamp", "leng"]
    del df["XXX"]

    df["Time"] = [i + 1 for i in range(len(df))]
    # sns.distplot(df["MBps"], hist=False, rug=False)
    # plt.savefig(target_file_name + "_dis.png")
    # plt.clf()
    # plt.close()
    # sns.distplot(df["bytes"], hist=False, rug=True)

    a = int(len(df) * 0.01)
    if a == 0:
        a = 1
    b = int(len(df) * 0.05)
    if b == 0:
        b = a + 1
    c = int(len(df) * 0.1)
    if c == 0:
        c = b + 1
    d = int(len(df) * 0.2)
    if d == 0:
        d = c + 1
    # print ("[a,b,c,d]", a, b, c, d)

    rolling_mean = df.MBps.rolling(window=a).mean()
    rolling_mean2 = df.MBps.rolling(window=b).mean()
    rolling_mean3 = df.MBps.rolling(window=c).mean()
    rolling_mean4 = df.MBps.rolling(window=d).mean()

    plt.plot(df.Time, rolling_mean, label='Mov.Avg Last %s instances' % a, color='orange')
    plt.plot(df.Time, rolling_mean2, label='Mov.Avg Last %s instances' % b, color='magenta')
    plt.plot(df.Time, rolling_mean3, label='Mov.Avg Last %s instances' % c, color='red')
    plt.plot(df.Time, rolling_mean4, label='Mov.Avg Last %s instances' % d, color='blue')

    plt.legend(loc='upper right')
    # plt.show()
    plt.xlabel("Time Instances")
    plt.ylabel("IO Speed in MB/s")
    lidex = onlyname.find(':')
    ridx = lidex + onlyname[lidex + 1:].find('.')
    try:
        thread_cnt = sys.argv[3]  # onlyname[lidex + 1:ridx + 1]
    except:
        thread_cnt = "dummy"
    plt.title('IO Throughput, ThreadCount {}'.format(thread_cnt), fontsize=14)
    # plt.show()
    plt.savefig(target_file_name)
    plt.close()

    rolling_mean = df.spent.rolling(window=a).mean()
    rolling_mean2 = df.spent.rolling(window=b).mean()
    rolling_mean3 = df.spent.rolling(window=c).mean()
    rolling_mean4 = df.spent.rolling(window=d).mean()

    plt.plot(df.Time, rolling_mean, label='Mov.Avg Last %s instances' % a, color='orange')
    plt.plot(df.Time, rolling_mean2, label='Mov.Avg Last %s instances' % b, color='magenta')
    plt.plot(df.Time, rolling_mean3, label='Mov.Avg Last %s instances' % c, color='red')
    plt.plot(df.Time, rolling_mean4, label='Mov.Avg Last %s instances' % d, color='blue')

    plt.legend(loc='upper right')
    # plt.show()
    plt.xlabel("Time Instances")
    plt.ylabel("Time Taken")
    lidex = onlyname.find(':')
    ridx = lidex + onlyname[lidex + 1:].find('.')
    # thread_cnt = sys.argv[3]  # onlyname[lidex + 1:ridx + 1]
    plt.title('Time Spent, ThreadCount {}'.format(thread_cnt), fontsize=14)
    # plt.show()
    plt.savefig(target_file_name + "_time.png")
    plt.close()

    rolling_mean = df.leng.rolling(window=a).mean()
    rolling_mean2 = df.leng.rolling(window=b).mean()
    rolling_mean3 = df.leng.rolling(window=c).mean()
    rolling_mean4 = df.leng.rolling(window=d).mean()

    plt.plot(df.Time, rolling_mean, label='Mov.Avg Last %s instances' % a, color='orange')
    plt.plot(df.Time, rolling_mean2, label='Mov.Avg Last %s instances' % b, color='magenta')
    plt.plot(df.Time, rolling_mean3, label='Mov.Avg Last %s instances' % c, color='red')
    plt.plot(df.Time, rolling_mean4, label='Mov.Avg Last %s instances' % d, color='blue')

    plt.legend(loc='upper right')
    # plt.show()
    plt.xlabel("Time Instances")
    plt.ylabel("Write Length")
    lidex = onlyname.find(':')
    ridx = lidex + onlyname[lidex + 1:].find('.')
    # thread_cnt = sys.argv[3]  # onlyname[lidex + 1:ridx + 1]
    plt.title('BYte length, ThreadCount {}'.format(thread_cnt), fontsize=14)
    # plt.show()
    plt.savefig(target_file_name + "_len.png")
    plt.close()

    len_dict = df.groupby('timestamp').mean()["leng"].to_dict()
    len_global_list.append((target_file_name,len_dict))

    time_df = {}
    for each in df["timestamp"]:
        old = time_df.get(each, 0)
        time_df[each] = old + 1
        oldg = df_global.get(each, 0)
        df_global[each] = oldg + 1

    df_global_list.append((target_file_name, time_df))


def main(file_path, prefix):
    if len(sys.argv) >= 4:
        prefix = os.sep.join([prefix, sys.argv[3]])
    if not os.path.exists(prefix):
        os.makedirs(prefix)
    for each in tqdm(os.listdir(file_path)):
        if '.csv' in each:
            try:
                target_file_name = os.sep.join([prefix, each + ".png"])
                mk_png_file(each, target_file_name, os.sep.join([file_path, each]))
            except Exception as ex:
                print("Error at {} ex={}".format(each, ex))
                raise Exception

    for _target_file_name, _len_df in len_global_list:
        for each_key in df_global.keys():
            if each_key not in _len_df.keys():
                _len_df[each_key] = 0
        len_df = sorted(_len_df.items(), key=lambda x: x[0], reverse=False)
        x = list(map(lambda x: x[0], len_df))
        x_min = min(x)
        x = [i - x_min for i in x]
        # plt.plot(x, np.cumsum(list(map(lambda x: x[1], time_df))), marker='o')
        plt.plot(x, list(map(lambda x: x[1], len_df)), marker='x')
        # plt.plot(list(map(lambda x: x[0], time_df)), list(map(lambda x: x[1], time_df)),marker='o')
        plt.xlabel("Timestamp (seconds)")
        plt.ylabel("Length")
        try:
            thrd_cnt = sys.argv[3]
        except:
            thrd_cnt = "dummy"
        plt.title('#ThreadCnt{}_{}'.format(thrd_cnt, _target_file_name), fontsize=14)
        plt.savefig(_target_file_name + "_len_psec.png")
        plt.close()


    for _target_file_name, _time_df in df_global_list:
        for each_key in df_global.keys():
            if each_key not in _time_df.keys():
                _time_df[each_key] = 0
        time_df = sorted(_time_df.items(), key=lambda x: x[0], reverse=False)
        x = list(map(lambda x: x[0], time_df))
        x_min = min(x)
        x = [i - x_min for i in x]
        # plt.plot(x, np.cumsum(list(map(lambda x: x[1], time_df))), marker='o')
        plt.plot(x, list(map(lambda x: x[1], time_df)), marker='x')
        # plt.plot(list(map(lambda x: x[0], time_df)), list(map(lambda x: x[1], time_df)),marker='o')
        plt.xlabel("Timestamp (seconds)")
        plt.ylabel("System Calls Count")
        try:
            thrd_cnt = sys.argv[3]
        except:
            thrd_cnt = "dummy"
        plt.title('#ThreadCnt{}_{}'.format(thrd_cnt, _target_file_name), fontsize=14)
        plt.savefig(_target_file_name + "_syscall.png")
        plt.close()

    time_df = sorted(df_global.items(), key=lambda x: x[0], reverse=False)
    x = list(map(lambda x: x[0], time_df))
    x_min = min(x)
    x = [i - x_min for i in x]
    plt.figure(figsize=(10, 8))
    plt.plot(x, list(map(lambda x: x[1], time_df)), marker='x', label='System Calls Count')
    plt.xlabel("Timestamp (seconds)")
    plt.ylabel("System Calls Count")
    plt.title('#ThreadCnt{}'.format(thrd_cnt), fontsize=14)
    plt.savefig(prefix + os.sep + "combined_syscall.png")
    plt.close()


if __name__ == '__main__':
    main(sys.argv[1], sys.argv[2])
    # main("/Users/mrityunjaykumar/USA/SBU/Summer2019/CSE 523/py/csv_logs/29",
    #      "/Users/mrityunjaykumar/USA/SBU/Summer2019/CSE 523/py/Out/")
