import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.2f' % (x * 1e-6)
formatter = FuncFormatter(millions)

matplotlib.rcParams['text.usetex'] = True
plt.rcParams['text.usetex'] = True

micro = """
463392	455310	511141.00
918547	910193	1026960.00
1.16E+06	1.16E+06	1351480.00
1.57E+06	1.61E+06	1868600.00
1.83E+06	1.87E+06	2206130.00
2.27E+06	2.27E+06	2716540.00
2.54E+06	2.53E+06	3004250.00
2.99E+06	3.00E+06	3566750.00
3.24E+06	3.28E+06	3859220.00
3.67E+06	3.67E+06	4396650.00
3.96E+06	3.91E+06	4670960.00
4.41E+06	4.30E+06	5285350.00
4.73E+06	4.61E+06	5516620.00
5.14E+06	4.91E+06	6126990.00
5.43E+06	5.22E+06	6414510.00
5.87E+06	5.67E+06	6970440.00
6.11E+06	5.94E+06	7298190.00
6.52E+06	6.32E+06	7904930.00
6.76E+06	6.57E+06	8109130.00
7.19E+06	6.98E+06	8727610.00
7.47E+06	7.18E+06	9065450.00
7.83E+06	7.60E+06	9540530.00
8.15E+06	7.82E+06	9758400.00
8.57E+06	8.20E+06	10367800.00
8.80E+06	8.44E+06	10636800.00
9.27E+06	8.84E+06	11347300.00
9.52E+06	9.13E+06	11622400.00
9.88E+06	9.45E+06	12008600.00
1.02E+07	9.79E+06	12438700.00
1.05E+07	1.02E+07	12945800.00
1.07E+07	1.03E+07	13282200.00
"""

# only show even numbers
keys, values, values2, values3, values0, values20, values30 = [], [], [], [], [], [], []

idx = 0
for l in micro.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 3:
        continue
    idx += 1
    if idx % 2 == 1:
        continue
    keys.append(idx)
    values0.append(float(items[0]))
    values20.append(float(items[1]))
    values30.append(float(items[2]))

# https://matplotlib.org/stable/tutorials/introductory/customizing.html
plt.rcParams["font.size"] = 60
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "Times"
matplotlib.rcParams["font.family"] = "Times"
fig, ax = plt.subplots(figsize=(16, 10))

ax.yaxis.set_major_formatter(formatter)
#ax.plot(keys, values0, marker="o", label='2-replica', linewidth=6)
ax.plot(keys, values20, marker="s", label='Rolis', linewidth=6)
ax.plot(keys, values30, marker="^", label='Silo', linewidth=6)
# ax.set_title("(a) YCSB++", y=-0.28, fontsize=32)
ax.set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax.set_xticklabels(["2", "", "", "8", "", "", "", "16", "", "", "", "24", "", "", "30"], fontsize=60)
ax.set_xlabel("\# of threads", fontsize=60, fontname="Times")

ax.set_yticks([0 * 10**6, 2.8 * 10**6, 5.6 * 10**6, 8.4 * 10**6, 11.2 * 10**6, 14 * 10**6])
ax.set_yticklabels(["0", "2.8", "5.6", "8.4", "11.2", "14"], fontsize=60)
ax.legend(bbox_to_anchor=(0.004, -0.01, 0.46, 1), mode="expand", ncol=1, loc="upper left", borderaxespad=0, frameon=True, fancybox=False, framealpha=1)

for tick in ax.get_xticklabels():
    tick.set_fontname("Times")
for tick in ax.get_yticklabels():
    tick.set_fontname("Times")
ax.yaxis.grid()

ax.xaxis.labelpad = 20
ax.yaxis.labelpad = 20

fig.tight_layout()
plt.subplots_adjust(bottom=0.18, left=0.1)
fig.savefig("exp_scalability_micro.eps", format='eps', dpi=1000)
plt.show()