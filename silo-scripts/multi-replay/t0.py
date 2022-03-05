import collections

if __name__ == "__main__":
    with open("./logs.log", "r") as handler:
        M = {} 
        cM = {}
        valid = 0
        for l in handler.readlines():
            l = l.strip()
            cid, k, v = int(l.split("_")[0]),int(l.split("_")[1]),int(l.split("_")[2]) 
            cM[cid] = 1  
            if "%s_%s" % (cid, k) in M:
                valid += 1
                print("%s_%s" % (cid, k))
            else:
                M["%s_%s" % (cid, k)] = 1
        print(valid, len(cM))
