import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter
matplotlib.rcParams['text.usetex'] = True
plt.rcParams['text.usetex'] = True
def millions(x, pos):
    return '%1.1fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

txt = """
85979.5	148381.09
166700	169824.04
193015	257370.97
272218	324356.1333
297835	413270.0767
372591	479370.8
400228	563165.64
477035	630534.3967
501271	716853.7733
572880	785961.4233
599828	869114.8767
671782	937227.36
695629	1024486.57
756467	1089593.203
782212	1173592.793
860598	1238620.8
890395	1318829.417
953467	1384527.03
977281	1466160.253
1039610	1.53E+06
1069010	1617509.227
1129720	1654777.143
1147720	1738533.38
1218920	1789326.49
1244000	1858560.077
1304050	1938815.917
1326880	1966345.74
1388910	2057199.467
1419160	2122027.617
1473780	2174341.017
1491820	2254354.057
"""
keys, values, values2 = [], [], []
idx = 0
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 2:
        continue
    idx += 1
    if idx % 2 == 1:
        continue
    keys.append(idx)
    values.append(float(items[0]))
    values2.append(float(items[1]))

values=[float(e) for e in values]
values2=[float(e) for e in values2]

plt.rcParams["font.size"] = 30
matplotlib.rcParams['lines.markersize'] = 14
plt.rcParams["font.family"] = "Times"
matplotlib.rcParams["font.family"] = "Times"
fig, ax = plt.subplots(figsize=(14, 9))

ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values, color="#1f77b4", marker="s", label='Silo', linewidth=3)
ax.plot(keys, values2, color="#ff7f0e", marker="^", label='Replay-only (Rolis)', linewidth=3)

ax.set_xticks([2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30])
ax.set_xticklabels([2, "", 6, "", 10, "", 14, "", 18, "", 22, "", 26, "", 30])
ax.set_ylim([0, 2.4 * 10**6])

# ax.set(xlabel='# of threads',
#        ylabel='Throughput (txns/sec)',
#        title=None)
ax.set_xlabel("\# of threads", fontname="Times")
ax.set_ylabel("Throughput (txns/sec)", fontname="Times")
ax.yaxis.grid()
ax.legend(bbox_to_anchor=(0, 0.92, 1, 0.2), mode="expand", ncol=2, loc="upper left", borderaxespad=0.2, frameon=False)

fig.tight_layout()
fig.savefig("exp_replay_only.eps", format='eps', dpi=1000)
plt.show()