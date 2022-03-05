import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.1fK' % (x * 1e-3)
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
    values.append(float(items[0])/(idx + 0.0))
    values2.append(float(items[1])/(idx + 0.0))
    values3.append(float(items[2])/(idx + 0.0))


plt.rcParams["font.size"] = 48
#matplotlib.rcParams['lines.markersize'] = 20
#matplotlib.rcParams["font.family"] = "serif"
#matplotlib.rcParams["font.family"] = "Times New Roman"
plt.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "serif"
#plt.rcParams["font.family"] = "Times New Roman"
fig, ax = plt.subplots(figsize=(16, 10))
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.family"] = "serif"
matplotlib.rcParams["font.family"] = "serif"

ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values2, marker="s", label='Rolis', linewidth=6)
ax.plot(keys, values3, marker="^", label='Silo', linewidth=6)

ax.set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax.set_xticklabels(["2", "", "", "8", "", "", "", "16", "", "", "", "24", "", "", "30"], fontsize=60)
ax.set_xlabel("# of threads", fontsize=60, fontname="serif")

ax.set_yticks([20 * 10**3, 34 * 10**3, 48 * 10**3, 62 * 10**3, 76 * 10**3, 90 * 10**3])
ax.set_yticklabels(["20", "34", "48", "62", "76", "90"], fontsize=60)
ax.legend(bbox_to_anchor=(0.538, -0.01, 0.46, 1), mode="expand", ncol=1, loc="upper right", borderaxespad=0, frameon=True, fancybox=False, framealpha=1)
ax.yaxis.grid()
for tick in ax.get_xticklabels():
    tick.set_fontname("serif")
for tick in ax.get_yticklabels():
    tick.set_fontname("serif")
fig.tight_layout()
fig.savefig("per_core_tpcc.eps", format='eps', dpi=1000)
plt.show()