import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

matplotlib.rcParams['text.usetex'] = True
plt.rcParams['text.usetex'] = True

def millions(x, pos):
    return '%1.1fM' % (x * 1e-6)
formatter = FuncFormatter(millions)


# To skewed workload
#   1. set scale-factor to 4
#   2. g_txn_workload_mix => {100, 0, 0, 0, 0} in tpcc.cc
#   3. g_new_order_fast_id_gen => 0 in tpcc.cc

txt = """
78142.8	64112.2
156715	127976
192041	152779
275643	214060
289982	229214
354948	278506
369024	290945
429444	336098
429493	339907
463318	370250
460503	369147
482133	389811
461604	379198
474686	391492
459766	379793
470161	387130
455336	371901
458591	380140
450329	370610
455064	376940
446246	367394
451524	372678
445698	360527
454408	369623
445999	363356
450963	371173
449862	366879
453550	369418
451609	366838
449656	368566
448397	371047
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
plt.rcParams["font.family"] = "Times"
matplotlib.rcParams["font.family"] = "Times"
fig, ax = plt.subplots(figsize=(14, 9))

ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values, marker="o", label='Silo', linewidth=3)
ax.plot(keys, values2, marker="s", label='Rolis', linewidth=3)
ax.set_ylim([0, 5 * 10 **5])

# ax.set(xlabel='# of threads',
#           ylabel='Throughput (txns/sec)',
#           title=None)
ax.set_xlabel("\# of threads", fontname="Times")
ax.set_ylabel("Throughput (txns/sec)", fontname="Times")
# https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
#ax.legend(bbox_to_anchor=(0.001, 0.799, 0.4, 0.2), mode="expand", ncol=1, loc="lower right", borderaxespad=0, frameon=True, fancybox=False, framealpha=1)
ax.legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=4, loc="upper left", borderaxespad=0.2, frameon=False)
ax.set_xticks([4, 8, 12, 16, 20, 24, 28])
ax.set_xticklabels(["4", "8", "12", "16", "20", "24", "28"])
ax.yaxis.grid()
for tick in ax.get_xticklabels():
    tick.set_fontname("Times")
for tick in ax.get_yticklabels():
    tick.set_fontname("Times")
fig.tight_layout()
fig.savefig("skewed.eps", format='eps', dpi=1000)
plt.show()