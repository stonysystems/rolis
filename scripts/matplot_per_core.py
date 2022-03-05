import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.1fK' % (x * 1e-3)
formatter = FuncFormatter(millions)

tpcc = """
89352.7	85807.8	107560
174187	167545	214613
198122	191751	246412
276521	269796	351269
302461	292732	383938
376884	360858	487254
405960	385456	519426
475641	452601	618487
499349	474632	651021
559656	538919	747774
595069	564021	783443
648229	617214	878188
681921	649657	9.08E+05
7.42E+05	699113	9.94E+05
7.68E+05	730758	1.04E+06
8.25E+05	7.88E+05	1.12E+06
8.47E+05	8.07E+05	1.16E+06
9.11E+05	8.58E+05	1.25E+06
9.34E+05	8.89E+05	1.28E+06
9.95E+05	9.36E+05	1.37E+06
1.01E+06	9.58E+05	1.40E+06
1.07E+06	1.01E+06	1.49E+06
1.08E+06	1.04E+06	1.51E+06
1.15E+06	1.08E+06	1.61E+06
1.17E+06	1.10E+06	1.64E+06
1.22E+06	1.14E+06	1.73E+06
1.24E+06	1.16E+06	1.75E+06
1.26E+06	1.20E+06	1.83E+06
1.29E+06	1.20E+06	1.85E+06
1.32E+06	1.24E+06	1.93E+06
1.33E+06	1.24E+06	1.95E+06
"""

micro = """
451741	438430	511141
866998	833957	1.03E+06
1.10E+06	1.08E+06	1.35E+06
1.51E+06	1.44E+06	1.87E+06
1.74E+06	1.73E+06	2.21E+06
2.18E+06	2.10E+06	2.72E+06
2.39E+06	2.35E+06	3.00E+06
2.82E+06	2.77E+06	3.57E+06
3.06E+06	3.00E+06	3.86E+06
3.53E+06	3.25E+06	4.40E+06
3.64E+06	3.58E+06	4.67E+06
4.17E+06	3.93E+06	5.29E+06
4.36E+06	4.19E+06	5.52E+06
4.74E+06	4.52E+06	6.13E+06
4.98E+06	4.80E+06	6.41E+06
5.25E+06	5.10E+06	6.97E+06
5.47E+06	5.41E+06	7.30E+06
5.84E+06	5.77E+06	7.90E+06
6.00E+06	5.88E+06	8.11E+06
6.37E+06	6.24E+06	8.73E+06
6.65E+06	6.45E+06	9.07E+06
6.88E+06	6.79E+06	9.54E+06
7.12E+06	7.01E+06	9.76E+06
7.45E+06	7.30E+06	1.04E+07
7.80E+06	7.60E+06	1.06E+07
8.00E+06	7.91E+06	1.13E+07
8.22E+06	7.97E+06	1.16E+07
8.48E+06	8.41E+06	1.20E+07
8.78E+06	8.60E+06	1.24E+07
9.21E+06	8.82E+06	1.29E+07
9.27E+06	9.12E+06	1.33E+07
"""

keys, values, values2, values3, values0, values20, values30 = [], [], [], [], [], [], []
idx = 0
for l in tpcc.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 3:
        continue
    idx += 1
    if idx % 2 == 1:
        continue
    keys.append(idx)
    values.append(float(items[0])/idx)
    values2.append(float(items[1])/idx)
    values3.append(float(items[2])/idx)

idx = 0
for l in micro.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 3:
        continue
    idx += 1
    if idx % 2 == 1:
        continue
    values0.append(float(items[0])/idx)
    values20.append(float(items[1])/idx)
    values30.append(float(items[2])/idx)

plt.rcParams["font.size"] = 20
matplotlib.rcParams['lines.markersize'] = 10
fig, ax = plt.subplots(nrows=1, ncols=2, figsize=(16, 8))
ax[0].yaxis.set_major_formatter(formatter)
ax[0].plot(keys, values, marker="o", label='2-replicas')
ax[0].plot(keys, values2, marker="s", label='3-replicas')
ax[0].plot(keys, values3, marker="*", label='Silo')

ax[0].set(xlabel='# of cores',
          ylabel='Throughput (txns/sec)',
          title=None)
ax[0].set_title("(a) TPC-C", y=-0.28, fontsize=32)
# https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
ax[0].legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=3, loc="upper left", borderaxespad=0.2, frameon=False)
ax[0].set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax[0].set_xticklabels(["2", "", "6", "", "10", "", "14", "", "18", "", "22", "", "26", "", "30"])
ax[0].yaxis.grid()

ax[1].yaxis.set_major_formatter(formatter)
ax[1].plot(keys, values0, marker="o", label='2-replicas')
ax[1].plot(keys, values20, marker="s", label='3-replicas')
ax[1].plot(keys, values30, marker="*", label='Silo')
ax[1].set(xlabel='# of cores',
          ylabel='Throughput (txns/sec)',
          title=None)
ax[1].set_title("(a) YCSB++", y=-0.28, fontsize=32)
ax[1].set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax[1].set_xticklabels(["2", "", "6", "", "10", "", "14", "", "18", "", "22", "", "26", "", "30"])
ax[1].legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=3, loc="upper left", borderaxespad=0.2, frameon=False)
ax[1].yaxis.grid()

fig.tight_layout()
fig.savefig("per_core.eps", format='eps', dpi=1000)
plt.show()