import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

matplotlib.rcParams['text.usetex'] = True
plt.rcParams['text.usetex'] = True

def millions(x, pos):
    return '%1.0fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

txt = """
4	381682	159258	1607310	1597470
8	773030	366397	3001100	2926390
12	1103917	519834	4297950	4153020
16	1358805	740740	5670450	5327070
20	1850419	884978	6978090	6580080
24	2227947	1066185	8195800	7775010
28	2587756	1223889	9449680	8875810
"""

keys, values, values2, values3, values4, values0, values20, values30 = [], [], [], [], [], [], [], []
idx = 0
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 5:
        continue
    values.append(float(items[1]))
    values2.append(float(items[2]))
    values3.append(float(items[3]))
    values4.append(float(items[4]))

keys = [4, 8, 12, 16, 20, 24, 28]

plt.rcParams["font.size"] = 30
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "Times"
matplotlib.rcParams["font.family"] = "Times"
fig, ax = plt.subplots(figsize=(14, 9))

ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values, marker="o", label='Meerkat - YCSB-T', linewidth=3)
ax.plot(keys, values2, marker="s", label='Meerkat - YCSB++', linewidth=3)
ax.plot(keys, values3, marker="^", label='Rolis - YCSB++', linewidth=3)
ax.plot(keys, values4, marker="x", label='Networked Rolis - YCSB++', linewidth=3)
ax.set_ylim([0, 11 * 10 **6])

# ax.set(xlabel='# of threads',
#           ylabel='Throughput (txns/sec)',
#           title=None)
ax.set_xlabel("\# of threads", fontname="Times")
ax.set_ylabel("Throughput (txns/sec)", fontname="Times")
# https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
ax.legend(bbox_to_anchor=(0.001, 0.799, 0.56, 0.2), mode="expand", ncol=1, loc="upper left", borderaxespad=0, frameon=True, fancybox=False, framealpha=1)
ax.set_xticks([4, 8, 12, 16, 20, 24, 28])
ax.set_xticklabels(["4", "8", "12", "16", "20", "24", "28"])
ax.yaxis.grid()
# ax.set_title("Rolis vs Meerkat", y=-0.28, fontsize=32)
for tick in ax.get_xticklabels():
    tick.set_fontname("Times")
for tick in ax.get_yticklabels():
    tick.set_fontname("Times")
ax.xaxis.labelpad = 20
ax.yaxis.labelpad = 20

fig.tight_layout()
plt.subplots_adjust(bottom=0.14)
fig.savefig("software_comparison_hardware.eps", format='eps', dpi=1000)
plt.show()