import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

# the figure with zero-based
def millions(x, pos):
    return '%1.2fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

# Batch-size	Throughput	10 %	50 %	90 %	95 %	99 %
txt = """
50	497411	0	14.896	25.122	26.056	0
100	562908	0	15.325	23.863	28.703	0
200	594331	0	17.439	26.794	31.835	0
400	617211	0	23.231	33.457	38.758	0
800	627155	0	35.637	48.544	55.539	0
1600	630986	0	64.903	90.554	109.251	0
3200	617875	0	128.171	187.753	228.898	0
"""

keys, values1, values2, values3, values4, values5, values6 = [], [], [], [], [], [], []
idx = 0
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 7:
        continue
    idx += 1
    keys.append(idx)
    values1.append(float(items[1]))
    values2.append(float(items[2]))
    values3.append(float(items[3]))
    values4.append(float(items[4]))
    values5.append(float(items[5]))
    values6.append(float(items[6]))

plt.rcParams["font.size"] = 30
matplotlib.rcParams['lines.markersize'] = 14
fig, ax = plt.subplots(figsize=(14, 9))
plt.rcParams["font.family"] = "serif"
matplotlib.rcParams["font.family"] = "serif"

ax.set_xticklabels(["50", "100", "200", "400", "800", "1600", "3200"])
ax.yaxis.set_major_formatter(formatter)
ax.set_xticks(keys)
# ax.set(xlabel='Batch size',
#         ylabel='Throughput (txns/sec)',
#         title=None)
ax.set_xlabel("Batch size", fontname="serif")
ax.set_ylabel("Throughput (txns/sec)", fontname="serif")
ax.plot(keys, values1, marker="+", label='TPUT', linestyle='--', linewidth=3)
ax.legend(bbox_to_anchor=(0, 0.92, 0.25, 0.2), mode="expand", ncol=1, loc="upper left", borderaxespad=0, frameon=False)
ax.set_ylim([0 * 1000, 700 * 1000])

ax2=ax.twinx()
ax2.set_xticks(keys)
ax2.plot(keys, values3, color="#1f77b4", marker="o", label='10th', linewidth=3)
ax2.plot(keys, values4, color="#ff7f0e", marker="s", label='50th', linewidth=3)
ax2.plot(keys, values5, color="#2ca02c", marker="^", label='95th', linewidth=3)
ax2.yaxis.grid()
# ax2.set(xlabel=None,
#         ylabel='Latency (ms)',
#         title=None)
ax2.set_ylabel("Latency (ms)", fontname="serif")
ax2.set_xticklabels(["50", "100", "200", "400", "800", "1600", "3200"])
ax2.legend(bbox_to_anchor=(0.25, 0.92, 0.75, 0.2), mode="expand", ncol=3, loc="upper left", borderaxespad=0, frameon=False, fancybox=False)
ax2.set_ylim([0, 240])
for tick in ax.get_xticklabels():
    tick.set_fontname("serif")
for tick in ax.get_yticklabels():
    tick.set_fontname("serif")
for tick in ax2.get_xticklabels():
        tick.set_fontname("serif")
for tick in ax2.get_yticklabels():
    tick.set_fontname("serif")
fig.tight_layout()
fig.savefig("latency-2.eps", format='eps', dpi=1000)
plt.show()