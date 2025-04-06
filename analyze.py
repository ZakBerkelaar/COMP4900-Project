#!/usr/bin/python
from enum import Enum
import re
import os
import sys

if len(sys.argv) != 2:
    print("Missing analysis path")
    exit(1)

NUM_TASKS = 8
PATH = sys.argv[1]

# In ms
task_arrival_times = [[] for _ in range(NUM_TASKS)]
task_start_times = [[] for _ in range(NUM_TASKS)]
task_end_times = [[] for _ in range(NUM_TASKS)]

def parse_file(f):
    output_region = False
    
    for line in f:
        if 'OUTPUT START' in line:
            output_region = True
        elif 'OUTPUT END' in line:
            output_region = False
        elif output_region:
            # We are reading output
            m = re.match(r'^(\d+) ms \| (\w+) task(\d+)$', line)
            if m is None:
                # No match
                continue
            
            time = int(m.group(1))
            action = m.group(2)
            task = int(m.group(3))

            if action == 'START':
                task_start_times[task].append(time)
            elif action == 'END':
                task_end_times[task].append(time)
            elif action == 'ARRIVE':
                task_arrival_times[task].append(time)

# Load in the files
for path in os.listdir(PATH):
    with open(os.path.join(PATH, path), 'r') as f:
        parse_file(f)

sets = len(task_start_times[0])

# Veryify that everything has the same number of sets
for i in range(NUM_TASKS):
    if sets != len(task_start_times[i]) or sets != len(task_end_times[i]) or sets != len(task_arrival_times[i]):
        print("Set length verification failed")
        exit(1)

print(f"Discovered {sets} sets of data")

# Find average makespan (total completion time)
makespan_sum = 0
for i in range(sets):
    # Find the max ending time for the set
    max_end = 0
    for j in range(NUM_TASKS):
        if (task_end_times[j][i] - task_arrival_times[j][i]) > max_end:
            max_end = (task_end_times[j][i] - task_arrival_times[j][i])

    makespan_sum = makespan_sum + max_end

average_makespan = makespan_sum / sets

# Find average turnaround time (task completion time)
turnaround_sum = 0
for i in range(sets):
    for j in range(NUM_TASKS):
        turnaround_sum = turnaround_sum + (task_end_times[j][i] - task_arrival_times[j][i])

average_turnaround = turnaround_sum / (sets * NUM_TASKS)

# Find average response time (time until first scheduled)
response_sum = 0
for i in range(sets):
    # Assume all tasks arrive at t = 0ms
    for j in range(NUM_TASKS):
        response_sum = response_sum + (task_start_times[j][i] - task_arrival_times[j][i])

average_response = response_sum / (sets * NUM_TASKS)

# Output the results
print(f"Average makespan: {average_makespan:0.2f}ms")
print(f"Average turnaround time: {average_turnaround:0.2f}ms")
print(f"Average response time: {average_response:0.2f}ms")