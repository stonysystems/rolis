import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.2fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

txt = """
74051.4
141290
160643
225656
245642
307685
325263
386773
404503
419650
421392
429043
424339
414874
424233
418594
419484
420718
426476
425430
419394
426826
424407
426031
428517
428194
424666
429158
430207
428846
425442
"""

keys, values = [], []
idx = 0
for e in txt.split("\n"):
    if not e:
        continue

    idx += 1
    if idx % 2 == 1:
        continue

    keys.append(idx)
    values.append(float(e))
matplotlib.rcParams['text.usetex'] = True
plt.rcParams['text.usetex'] = True

plt.rcParams["font.size"] = 30
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "Times"
matplotlib.rcParams["font.family"] = "Times"
fig, ax = plt.subplots(figsize=(14, 9))
ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values, marker="s", label='one Paxos Stream', linewidth=3)

# ax.set(xlabel='# of threads',
#        ylabel='Throughput (txns/sec)',
#        title=None)
ax.set_xlabel("\# of threads", fontname="Times")
ax.set_ylabel("Throughput (txns/sec)", fontname="Times")
ax.set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax.set_xticklabels(["2", "", "6", "", "10", "", "14", "", "18", "", "22", "", "26", "", "30"])
ax.legend(bbox_to_anchor=(0, 0.92, 1, 0.22), mode="expand", ncol=1, loc="upper left", borderaxespad=0.2, frameon=False)
ax.yaxis.grid()
for tick in ax.get_xticklabels():
    tick.set_fontname("Times")

ax.xaxis.labelpad = 20
ax.yaxis.labelpad = 20

fig.tight_layout()
plt.subplots_adjust(bottom=0.14)
fig.savefig("single_paxos_group.eps", format='eps', dpi=1000)
plt.show()
