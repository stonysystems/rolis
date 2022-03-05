import matplotlib
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.ticker import FuncFormatter

def millions(x, pos):
    return '%1.1fM' % (x * 1e-6)
formatter = FuncFormatter(millions)

txt="""
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
values, values2, values3 = [], [], []
for l in txt.split("\n"):
    items = l.replace("\n", "").split("\t")
    if len(items) != 3:
        continue
    values.append(float(items[0]))
    values2.append(float(items[1]))
    values3.append(float(items[2]))

values=[float(e) for e in values]
keys=[i+1 for i in range(len(values))]

values2=[float(e) for e in values2]
keys2=[i+1 for i in range(len(values))]

values3=[float(e) for e in values3]
keys3=[i+1 for i in range(len(values))]

plt.rcParams["font.size"] = 16
matplotlib.rcParams['lines.markersize'] = 10
fig, ax = plt.subplots(figsize=(10, 6))
ax.yaxis.set_major_formatter(formatter)
ax.plot(keys, values, marker="o", label='2-replicas')
ax.plot(keys2, values2, marker="s", label='3-replicas')
ax.plot(keys3, values3, marker="*", label='Silo')

ax.set(xlabel='# of cores - YCSB++',
       ylabel='throughput',
       title=None)
ax.legend()  # loc="upper right"
ax.grid()

fig.savefig("micro_scalability.eps", format='eps', dpi=1000)
plt.show()