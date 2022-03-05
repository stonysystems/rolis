from subprocess import call

if __name__ == "__main__":
    results = "./results-48M/results.log"
    outputs = "./results_convert.log"
    M = {}
    prev = []
    with open(results, "r") as handler:
        for idx, line in enumerate(handler.readlines()):
            items = line.strip().split("\t")
            if idx % 2 == 1:
                M[prev[0]] = items
            prev = items
    print(M)
    start, end = 1, 28

    wr = open(outputs, "w")
    ans = [""] * 28
    for i in range(start, end+1):
        for j in range(1, 3+1):
            key = "%s-%s" % (i, j)
            if j == 3:
                ans[i-1] += "%s\t%s\t%s" % (M[key][0], M[key][1], M[key][2])
            else:
                ans[i-1] += "%s\t%s\t%s\t" % (M[key][0], M[key][1], M[key][2])

    ctx = ""
    for i in range(len(ans)):
        ctx += ans[i] + "\n"
    wr.write(ctx)
    wr.close()

