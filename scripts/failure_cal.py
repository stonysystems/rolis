
import collections


def parse(e):
    items = e.split("] # of commits: ")
    return int(items[0].replace("[time: ", "")), int(items[1])


def aggregate(leader, follower):
    leader = [parse(e) for e in leader.split("\n") if e.startsWith("[time:")]
    follower = [parse(e) for e in follower.split("\n") if e.startsWith("[time:")]
    base_time = leader[0][0]
    leader = [(e[0]-base_time, e[1]) for e in leader]
    follower = [(e[0]-base_time, e[1]) for e in follower]

    throughput = collections.defaultdict(list)

    for i, (t, v) in enumerate(leader):
        throughput[i // 10].append((t, v))

    cut = (follower[0][0] // 1000 + 1) * 1000
    idx = follower[0][0] // 1000
    num = 0
    for e in follower:
        if e[0] <= cut:
            throughput[idx].append(e)
        else:
            throughput[idx + num//10+1].append(e)
            num += 1

    # print the throughput
    for t in range(50):
        if t in throughput:
            v = throughput[t]
            # print(v, len(v))
            if t - 1 in throughput:
                print((v[-1][1] - throughput[t-1][-1][1]))
            else:
                print(v[-1][1])
        else:
            print(0)


def read_grained(leader, follower):
    leader = [parse(e) for e in leader.split("\n") if e.startswith("[time:")]
    follower = [parse(e) for e in follower.split("\n") if e.startswith("[time:")]
    base_time = leader[0][0]
    leader = [(e[0]-base_time, e[1]) for e in leader]
    follower = [(e[0]-base_time, e[1]) for e in follower]

    throughput = collections.defaultdict(int)
    for i, (t, v) in enumerate(leader):
        throughput[i] = v

    idx = follower[0][0] // 100
    for e in follower:
        throughput[idx] = e[1]
        idx += 1

    res = []
    for t in range(300):
        if t in throughput:
            if t-1 in throughput:
                res.append(10*(throughput[t] - throughput[t-1]))
            else:
                res.append(throughput[t]*10)
        else:
            res.append(0)
    return res


if __name__ == "__main__":
    with open("./scripts/failure_leader") as ll:
        leader = ll.read()

    with open("./scripts/failure_follower") as ff:
        follower = ff.read()

    # aggregate(leader, follower)
    res = read_grained(leader, follower)
    for e in res:
        print(e)
