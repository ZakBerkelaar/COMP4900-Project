#!/usr/bin/python
import matplotlib.pyplot as plt
from matplotlib import colors as mcolors
import re

fig, ax = plt.subplots()

COLORS = list(mcolors.TABLEAU_COLORS.values())

print(COLORS)

#lst = [(3, 0), (2, 10), (1, 20), (0, 30), (4, 40), (5, 50), (6, 60), (7, 70), (0, 80), (3, 90), (-1, 100)]
lst = []

with open('data/default_switching.log', 'r') as f:
    for line in f:
        m = re.match(r'^(\d+) \| Core0: BWorker(\d+)$', line)
        if m is None:
            continue

        tick = int(m.group(1))
        task = int(m.group(2))

        lst.append((task, tick))

#Append finalizing mark
# lst.append((-1, 101039)) # Used for EDF
# lst.append((-1, 1001)) # Used for LLREF
lst.append((-1, 251))

for i, x in enumerate(lst):
    if x[0] == -1:
        continue
    ax.barh(0, width=lst[i+1][1] - lst[i][1], left=x[1], color=COLORS[x[0]])

tasks = list(set([a for (a, b) in lst if a != -1]))

print(f'Tasks: {tasks}')

fig.set_figheight(0.5)
fig.set_figwidth(8)

plt.title('CPU0 Tasks (Default)')
plt.legend([f'Task {i}' for i in tasks], bbox_to_anchor=(1.04, 1))
leg = ax.get_legend()
for (idx, task) in enumerate(tasks):
    leg.legend_handles[idx].set_color(COLORS[task])

plt.xlabel("Time (ms)")
ax.yaxis.set_visible(False)
#plt.show()
plt.savefig('./test.png', bbox_inches='tight')