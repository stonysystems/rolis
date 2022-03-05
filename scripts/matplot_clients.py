import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.2fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

txt = """
"""

keys, values, values2 = [], [], []
idx = 0
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 2:
        continue
    idx += 1
    if idx % 2 != 0:
        continue
    keys.append(idx)
    values.append(float(items[0]))
    values2.append(float(items[1] or '0'))

plt.rcParams["font.size"] = 30
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "serif"
matplotlib.rcParams["font.family"] = "serif"
fig, ax = plt.subplots(figsize=(14, 9))

ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values, marker="o", label='Rolis (embedded client)', linewidth=3)
ax.plot(keys, values2, marker="s", label='Rolis (networked client)', linewidth=3)
ax.set_ylim([0, 11 * 10 **6])

# ax.set(xlabel='# of threads',
#           ylabel='Throughput (txns/sec)',
#           title=None)
ax.set_xlabel("# of threads", fontname="serif")
ax.set_ylabel("Throughput (txns/sec)", fontname="serif")
# https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
#ax.legend(bbox_to_anchor=(0.001, 0.799, 0.4, 0.2), mode="expand", ncol=1, loc="lower right", borderaxespad=0, frameon=True, fancybox=False, framealpha=1)
ax.legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=4, loc="upper left", borderaxespad=0.2, frameon=False)
ax.set_xticks([4, 8, 12, 16, 20, 24, 28, 30])
ax.set_xticklabels(["4", "8", "12", "16", "20", "24", "28", "30"])
ax.yaxis.grid()
for tick in ax.get_xticklabels():
    tick.set_fontname("serif")
for tick in ax.get_yticklabels():
    tick.set_fontname("serif")
fig.tight_layout()
fig.savefig("client.eps", format='eps', dpi=1000)
plt.show()
