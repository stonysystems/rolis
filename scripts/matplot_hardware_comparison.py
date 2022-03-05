import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.1fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

txt = """
4	381682	159258	1.61E+06
8	773030	366397	3.00E+06
12	1103917	519834	4.30E+06
16	1358805	740740	5.67E+06
20	1850419	884978	6.98E+06
24	2227947	1066185	8.20E+06
28	2587756	1223889	9.45E+06
"""

keys, values, values2, values3, values0, values20, values30 = [], [], [], [], [], [], []
idx = 0
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 4:
        continue
    values.append(float(items[1]))
    values2.append(float(items[2]))
    values3.append(float(items[3]))

keys = [4, 8, 12, 16, 20, 24, 28]

plt.rcParams["font.size"] = 30
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "serif"
matplotlib.rcParams["font.family"] = "serif"
fig, ax = plt.subplots(figsize=(14, 9))

ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values, marker="o", label='Meerkat - YCSB-T', linewidth=3)
ax.plot(keys, values2, marker="s", label='Meerkat - YCSB++', linewidth=3)
ax.plot(keys, values3, marker="^", label='Rolis - YCSB++', linewidth=3)
ax.set_ylim([0, 11 * 10 **6])

# ax.set(xlabel='# of threads',
#           ylabel='Throughput (txns/sec)',
#           title=None)
ax.set_xlabel("# of threads", fontname="serif")
ax.set_ylabel("Throughput (txns/sec)", fontname="serif")
# https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
ax.legend(bbox_to_anchor=(0.001, 0.799, 0.56, 0.2), mode="expand", ncol=1, loc="upper left", borderaxespad=0, frameon=True, fancybox=False, framealpha=1)
ax.set_xticks([4, 8, 12, 16, 20, 24, 28])
ax.set_xticklabels(["4", "8", "12", "16", "20", "24", "28"])
ax.yaxis.grid()
# ax.set_title("Rolis vs Meerkat", y=-0.28, fontsize=32)
for tick in ax.get_xticklabels():
    tick.set_fontname("serif")
for tick in ax.get_yticklabels():
    tick.set_fontname("serif")
fig.tight_layout()
fig.savefig("software_comparison_hardware.eps", format='eps', dpi=1000)
plt.show()