import collections
import sys

if __name__ == "__main__":
    import sys
    results_file = sys.argv[1]
    values = collections.defaultdict(list) 
    with open(results_file) as h:
        trd, trial = "", "1"
        for l in h.readlines():
            l = l.strip()
            if "-" in l:
              items = [e for e in l.split("-") if e.replace("\n", "").replace(" ", "") != ""]
              if len(items) != 2:
                print("err parse: ", l)
                sys.exit()
              trd, trial = items[0], items[1]
            else: # n_commits elapse throughput
              items = [e for e in l.split("\t") if e.replace("\n", "").replace(" ", "") != ""]
              if len(items) != 3:
                print("err parse: ", l)
                sys.exit()
              values[int(trd)].append((items[2])) 


    for trd in sorted(values.keys()):
        print("{trd}\t{trials}".format(trd=trd, trials="\t".join(list(values[trd]))))
