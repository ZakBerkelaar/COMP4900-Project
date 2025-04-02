#!/usr/bin/python
import matplotlib.pyplot as plt
from matplotlib import colors as mcolors

fig, ax = plt.subplots()

COLORS = list(mcolors.TABLEAU_COLORS.values())

lst = [(1, 0), (2, 10), (3, 20), (1, 30), (2, 40), (3, 50), (-1, 60)]



for i, x in enumerate(lst):
    if x[0] == -1:
        continue
    ax.barh(0, width=lst[i+1][1] - lst[i][1], left=x[1], color=COLORS[x[0]])

tasks = list(set([a for (a, b) in lst if a != -1]))

fig.set_figheight(0.5)
plt.legend([f'Task {i}' for i in tasks], bbox_to_anchor=(1.04, 1))
plt.xlabel("Time (ms)")
ax.yaxis.set_visible(False)
#plt.show()
plt.savefig('./test.png', bbox_inches='tight')