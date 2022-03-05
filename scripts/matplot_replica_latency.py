import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

# batch_size: 1000
txt = """
15.693	20.118	26.305
16.728	21.263	30.034
20.747	24.656	33.535
21.881	27.764	45.687
22.706	26.564	37.827
25.862	35.331	57.409
26.458	34.28	60.539
28.297	39.877	69.167
25.746	31.164	47.886
27.792	36.71	63.884
29.001	38.944	74.453
28.865	37.458	69.482
28.654	36.456	60.625
28.753	36.293	57.437
29.631	37.473	58.628
29.515	37.05	58.338
30.446	38.583	60.713
30.407	37.889	58.734
31.65	39.888	61.355
31.276	38.711	59.108
32.401	40.372	60.551
31.71	39.268	59.641
33.018	41.275	62.991
32.976	40.667	59.673
34.165	42.196	63.306
34.348	42.635	64.96
35.223	43.693	67.912
36.231	45.256	78.743
37.291	46.361	91.177
38.846	48.641	94.331
"""

# using batch_size: 500 (10%), 500 (50%), 500 (95%), 2000 (10%), 2000 (50%), 2000 (95%),
bound="""
8.366	10.71	13.887	32.13	40.961	52.746
9.345	12.086	16.755	32.885	40.517	55.713
10.942	13.545	18.51	40.216	47.578	65.032
12.048	16.013	25.812	41.742	51.569	89.371
12.541	15.459	22.024	43.449	49.599	72.054
13.847	18.644	31.32	46.333	59.079	99.096
13.875	17.575	27.867	46.513	55.054	85.578
15.721	21.823	39.216	46.604	56.257	89.447
16.513	23.044	46.161	53.329	72.62	139.241
16.994	23.652	44.533	50.039	62.41	104.613
16.093	21.369	36.236	51.175	63.15	105.714
17.409	23.692	40.43	51.584	65.427	107.251
17.118	22.702	37.758	52.038	63.762	126.692
17.818	23.901	39.502	51.632	62.198	93.069
17.048	22.397	36.808	54.146	66.379	105.681
17.347	22.98	36.773	53.557	65.18	99.716
17.841	23.459	38.046	55.773	68.421	109.544
18.752	25.001	39.88	54.845	65.996	97.272
18.385	24.173	39.513	56.793	68.826	107.75
18.633	24.327	37.773	57.176	69.041	104.37
19.284	25.363	39.525	57.852	69.735	110.674
19.847	25.809	40.871	58.019	69.885	109.904
19.822	25.994	40.86	60.774	73.899	117.742
19.997	26.119	39.746	60.071	72.261	116.606
20.527	26.492	41.047	61.741	74.137	110.625
20.376	26.42	46.262	62.291	76.27	130.458
21.266	27.737	48.693	64.339	78.02	153.74
21.431	27.943	77.782	66.043	79.527	138.372
21.948	28.398	75.616	69.228	83.486	172.158
22.766	29.652	82.355	77.976	93.591	171.447
"""

keys, values1, values2, values3 = [], [], [], []
idx = 0
for l in txt.split("\n"):
    idx += 1
    items = l.replace("\n", "").split("\t")
    if len(items) != 3:
        continue
    if idx % 2 == 1:
        continue
    keys.append(idx)
    values1.append(float(items[0]))
    values2.append(float(items[1]))
    values3.append(float(items[2]))

bvalue1, bvalue2, bvalue3, bvalue4, bvalue5, bvalue6 = [], [], [], [], [], []
idx = 0
for l in bound.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 6:
        continue
    idx += 1
    if idx % 2 == 1:
        continue
    bvalue1.append(float(items[0]))
    bvalue2.append(float(items[1]))
    bvalue3.append(float(items[2]))
    bvalue4.append(float(items[3]))
    bvalue5.append(float(items[4]))
    bvalue6.append(float(items[5]))

plt.rcParams["font.size"] = 20
matplotlib.rcParams['lines.markersize'] = 10
fig, ax = plt.subplots(nrows=1, ncols=2, figsize=(16, 8))

ax[0].plot(keys, values1, color="#1f77b4", marker="o", label='10th')
#ax.fill_between(keys, bvalue1, bvalue4, facecolor='#1f77b4', alpha=0.2)
ax[0].plot(keys, values2, color="#ff7f0e", marker="s", label='50th')
#ax.fill_between(keys, bvalue2, bvalue5, facecolor='#ff7f0e', alpha=0.2)
ax[0].plot(keys, values3, color="#2ca02c", marker="*", label='95th')
#ax.fill_between(keys, bvalue3, bvalue6, facecolor='#2ca02c', alpha=0.2)

ax[0].set(xlabel='# of threads',
       ylabel='latency (milliseconds)',
       title=None)
ax[0].legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=3, loc="upper left", borderaxespad=0.2, frameon=False)
ax[0].set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax[0].set_xticklabels([2, "", 6, "", 10, "", 14, "", 18, "", 22, "", 26, "", 30])
ax[0].yaxis.grid()
ax[0].set_title("(a) batch-size: 1000", y=-0.28, fontsize=32)
ax[0].set_ylim([0, 200])

ax[1].plot(keys, bvalue4, color="#1f77b4", marker="o", label='10th')
ax[1].plot(keys, bvalue5, color="#ff7f0e", marker="s", label='50th')
ax[1].plot(keys, bvalue6, color="#2ca02c", marker="*", label='95th')

ax[1].set(xlabel='# of threads',
          ylabel='latency (milliseconds)',
          title=None)
ax[1].legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=3, loc="upper left", borderaxespad=0.2, frameon=False)
ax[1].set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax[1].set_xticklabels([2, "", 6, "", 10, "", 14, "", 18, "", 22, "", 26, "", 30])
ax[1].yaxis.grid()
ax[1].set_title("(a) batch-size: 2000", y=-0.28, fontsize=32)
ax[1].set_ylim([0, 200])

fig.tight_layout()
fig.savefig("replica_latency.eps", format='eps', dpi=1000)
plt.show()