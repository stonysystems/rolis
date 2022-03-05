import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.1fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

tpcc = """
89352.70	85807.80	107560.00
174187.00	167545.00	214613.00
198122.00	191751.00	246412.00
276521.00	269796.00	351269.00
302461.00	292732.00	383938.00
376884.00	360858.00	487254.00
405960.00	385456.00	519426.00
475641.00	452601.00	618487.00
499349.00	474632.00	651021.00
559656.00	538919.00	747774.00
595069.00	564021.00	783443.00
648229.00	617214.00	878188.00
681921.00	649657.00	908210.00
742488.00	699113.00	994328.00
767960.00	730758.00	1035300.00
824539.00	788437.00	1124710.00
846596.00	807017.00	1156980.00
910919.00	858126.00	1245000.00
933846.00	888529.00	1279740.00
994711.00	935598.00	1369970.00
1011710.00	957550.00	1400020.00
1070100.00	1005650.00	1491090.00
1083850.00	1035730.00	1513200.00
1148890.00	1080890.00	1609500.00
1169510.00	1101100.00	1639010.00
1219020.00	1140290.00	1726920.00
1236490.00	1162260.00	1745930.00
1263550.00	1195050.00	1832190.00
1288060.00	1203580.00	1852420.00
1319780.00	1240020.00	1925580.00
1331630.00	1243640.00	1953420.00
"""

micro = """
451741.00	438430.00	511141.00
866998.00	833957.00	1026960.00
1102660.00	1075270.00	1351480.00
1509890.00	1444730.00	1868600.00
1742560.00	1731940.00	2206130.00
2180860.00	2104580.00	2716540.00
2388860.00	2354100.00	3004250.00
2821450.00	2765210.00	3566750.00
3060330.00	2998360.00	3859220.00
3531190.00	3249780.00	4396650.00
3642220.00	3576060.00	4670960.00
4167410.00	3925890.00	5285350.00
4363160.00	4192700.00	5516620.00
4735750.00	4521250.00	6126990.00
4982370.00	4796910.00	6414510.00
5251950.00	5101620.00	6970440.00
5470540.00	5412870.00	7298190.00
5836900.00	5766150.00	7904930.00
5997370.00	5877800.00	8109130.00
6368740.00	6244140.00	8727610.00
6649320.00	6447120.00	9065450.00
6883560.00	6785950.00	9540530.00
7124550.00	7013890.00	9758400.00
7451620.00	7298020.00	10367800.00
7798810.00	7596160.00	10636800.00
8001580.00	7914040.00	11347300.00
8222030.00	7969930.00	11622400.00
8482450.00	8408780.00	12008600.00
8784810.00	8601830.00	12438700.00
9212360.00	8824190.00	12945800.00
9273660.00	9119290.00	13282200.00
"""

# only show even numbers
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
    values.append(float(items[0]))
    values2.append(float(items[1]))
    values3.append(float(items[2]))

idx = 0
for l in micro.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 3:
        continue
    idx += 1
    if idx % 2 == 1:
        continue
    values0.append(float(items[0]))
    values20.append(float(items[1]))
    values30.append(float(items[2]))

# https://matplotlib.org/stable/tutorials/introductory/customizing.html
plt.rcParams["font.size"] = 20
matplotlib.rcParams['lines.markersize'] = 10
fig, ax = plt.subplots(nrows=1, ncols=2, figsize=(16, 8))
ax[0].yaxis.set_major_formatter(formatter)
ax[0].plot(keys, values, marker="o", label='2-replicas')
ax[0].plot(keys, values2, marker="s", label='3-replicas')
ax[0].plot(keys, values3, marker="*", label='Silo')

ax[0].set(xlabel='# of threads',
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
ax[1].set(xlabel='# of threads',
          ylabel='Throughput (txns/sec)',
          title=None)
ax[1].set_title("(a) YCSB++", y=-0.28, fontsize=32)
ax[1].set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax[1].set_xticklabels(["2", "", "6", "", "10", "", "14", "", "18", "", "22", "", "26", "", "30"])
ax[1].legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=3, loc="upper left", borderaxespad=0.2, frameon=False)
ax[1].yaxis.grid()

fig.tight_layout()
fig.savefig("exp_scalability.eps", format='eps', dpi=1000)
plt.show()