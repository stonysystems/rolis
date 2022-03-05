from subprocess import call

if __name__ == "__main__":
    start, end = 1, 28

    for i in range(start, end+1):
        for j in range(1, 3+1):
            try:
                cmd = "rm ./results/{thread_num}-{trial}.pdf".format(thread_num=i, trial=j)
                print(cmd)
                cmd = "google-pprof --pdf ../out-perf.masstree/benchmarks/ht_mt2 ./results/{thread_num}-{trial}.ht_mt2.prof > ./results/{thread_num}-{trial}.pdf".format(thread_num=i, trial=j)
                print(cmd)
            except Exception as ex:
                print(ex)