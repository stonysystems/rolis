import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.2f' % (x * 1e-6)
formatter = FuncFormatter(millions)

tpcc = """
1	73458.6	85979.5
2	138098	166700
3	155335	193015
4	214993	272218
5	235452	297835
6	290432	372591
7	313898	400228
8	363808	477035
9	382929	501271
10	432299	572880
11	454606	599828
12	503084	671782
13	523661	695629
14	570765	756467
15	578270	782212
16	633132	860598
17	648907	890395
18	696570	953467
19	713145	977281
20	754722	1039610
21	778280	1069010
22	817166	1129720
23	837805	1147720
24	876260	1218920
25	892306	1244000
26	928791	1304050
27	940796	1326880
28	978389	1388910
29	988470	1419160
30	1018090	1473780
31	1026620	1491820
"""

matplotlib.rcParams['text.usetex'] = True
plt.rcParams['text.usetex'] = True

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


# plt.rcParams["font.size"] = 24
# matplotlib.rcParams['lines.markersize'] = 24
# fig, ax = plt.subplots(figsize=(10, 10))

plt.rcParams["font.size"] = 60
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "Times"
matplotlib.rcParams["font.family"] = "Times"
fig, ax = plt.subplots(figsize=(16, 10))

ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values2, marker="s", label='Rolis', linewidth=6)
ax.plot(keys, values3, marker="^", label='Silo', linewidth=6)

# https://stackoverflow.com/questions/4700614/how-to-put-the-legend-out-of-the-plot/43439132#43439132
ax.set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax.set_xticklabels(["2", "", "", "8", "", "", "", "16", "", "", "", "24", "", "", "30"], fontsize=60)
ax.set_xlabel("\# of threads", fontsize=60, fontname="Times")

ax.set_yticks([0 * 10**6, 0.3 * 10**6, 0.6 * 10**6, 0.9 * 10**6, 1.2 * 10**6, 1.5 * 10**6])
ax.set_yticklabels(["0", "0.3", "0.6", "0.9", "1.2", "1.5"], fontsize=60)
ax.legend(bbox_to_anchor=(0.004, -0.01, 0.4, 1), mode="expand", ncol=1, loc="upper left", borderaxespad=0, frameon=True, fancybox=False, framealpha=1)
for tick in ax.get_xticklabels():
    tick.set_fontname("Times")
for tick in ax.get_yticklabels():
    tick.set_fontname("Times")
ax.yaxis.grid()
ax.xaxis.labelpad = 20
ax.yaxis.labelpad = 20
# ax.margins(x=0)
# plt.margins(x=0)

fig.tight_layout()
# plt.tight_layout()
plt.subplots_adjust(bottom=0.18, left=0.1)
fig.savefig("exp_scalability_tpcc.eps", format='eps', dpi=1000)
plt.show()