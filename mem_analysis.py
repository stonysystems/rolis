import psutil
import time
import sys
import getopt
import datetime


def analysis(run_log_leader, ds0, ds1, ds2):
    len_of_cpu = 17

    # get the start_point, end_point
    times, throughput=[],0
    with open(run_log_leader) as h:
        for l in h.readlines():
            # [time: 1645110144635] # of commits: 16187904
            if "[time" in l.strip():
                times.append(l.strip().split("]")[0].replace("[time:", ""))
            if "agg_throughput:" in l.strip():
                throughput=float(l.strip().replace("agg_throughput:", "").replace("ops/sec", ""))  
                
    start_point, end_point = float(times[0])/1000.0,float(times[-1])/1000.0
    #start_point, end_point = 1644865722, 1644865729 
            
    print("throughput:", throughput)
    for d in [ds0, ds1, ds2]:
        if not d: continue
        values = []
        skip_header=0
        # time	mem_usage	node0   cpu0 ~ cpu31
        with open(d) as handler:
            for l in handler.readlines():
                if skip_header:
                    values.append(l.split("\t"))
                else:
                    skip_header=1
        all_mem=[]
        all_cpu=[]
        for v in values:
            cur=int(v[0])
            if cur>=start_point and cur<=end_point:
                tmp=[]
                for c in v[3:3+len_of_cpu]:
                    tmp.append(float(c))
                # do a special processing for the leader
                if d == "ds0.log" and sum(tmp)/len(tmp) <= 10:
                    continue
                s_idx,s_v=-1,float("inf")
                for i,t in enumerate(tmp):
                    if t<s_v:
                        s_idx,s_v=i,t
                tmp=tmp[0:s_idx]+tmp[s_idx+1:]
                all_cpu+=tmp
                all_mem.append(float(v[1]))
        print("{f0}\t{f1}\t{f2}".format(f0=d,f1=sum(all_mem)/len(all_mem), f2=sum(all_cpu)/len(all_cpu))) 


if __name__ == "__main__":
    import sys
    # run_log_leader, ds0, ds1, ds2
    files = sys.argv[1:]
    if not (len(files)==4):
        print("err input: ", sys.argv[1:])
    
    run_log_leader,ds0,ds1,ds2=files[0], files[1], files[2], files[3]
    analysis(run_log_leader, ds0, ds1, ds2)
