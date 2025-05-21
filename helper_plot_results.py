#!/usr/bin/env python3
# filepath: latency_analysis.py

import re
import numpy as np
import matplotlib.pyplot as plt

# Sample log data - this could  be read from a file in practice
log_data = """
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 0 addr 0x0001 avg latency:  78 ms failures 0 # values: 59 105 63 80 133 86 83 60 84 77 65 63 76 79 87 83 76 78 54 82
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 1 addr 0x0002 avg latency:  85 ms failures 0 # values: 81 47 72 70 77 71 92 73 75 98 75 76 78 78 127 100 124 77 79 147
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 2 addr 0x0003 avg latency:  70 ms failures 0 # values: 63 41 63 45 74 73 46 96 91 70 88 97 45 67 97 98 97 43 66 43
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 3 addr 0x0004 avg latency:  67 ms failures 0 # values: 88 89 65 58 39 89 40 36 92 66 36 56 64 38 37 95 95 36 169 61
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 4 addr 0x0005 avg latency:  58 ms failures 0 # values: 58 29 31 51 63 53 87 61 51 78 55 76 74 31 82 30 90 109 30 31
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 5 addr 0x0006 avg latency:  42 ms failures 0 # values: 50 74 44 25 23 24 70 46 25 51 51 23 52 24 24 67 53 24 47 53
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 6 addr 0x0007 avg latency:  37 ms failures 0 # values: 19 41 18 45 42 46 63 46 18 19 72 18 49 18 40 18 76 41 43 18
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 7 addr 0x0008 avg latency:  20 ms failures 0 # values: 13 12 37 12 39 12 11 35 32 12 12 12 34 12 36 13 13 12 38 12
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 8 addr 0x0009 avg latency:   6 ms failures 0 # values: 5 7 6 6 5 6 7 6 6 6 6 6 6 6 6 5 6 6 6 7
d_09: @00:04:09.435151  [00:04:09.435,150] <inf> mesh_nw_test: Dev 9 addr 0x000a avg latency:   0 ms failures 0 # values: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
"""

def parse_latency_data(log_content):
    latency_data = {}

    # extract device number, address, and latency values
    pattern = r'Dev (\d+) addr (0x[0-9a-f]+) avg latency:\s+(\d+) ms failures \d+ # values: ([\d\s]+)'

    matches = re.finditer(pattern, log_content)
    for match in matches:
        dev_num = int(match.group(1))
        address = match.group(2)
        avg_latency = int(match.group(3))
        latency_values = [int(x) for x in match.group(4).strip().split()]

        latency_data[dev_num] = {
            'address': address,
            'avg_latency': avg_latency,
            'values': latency_values
        }

    return latency_data

def create_violinplot(latency_data):
    fig, ax = plt.subplots(figsize=(12, 8))

    dev_nums = sorted(latency_data.keys())
    data = [latency_data[dev]['values'] for dev in dev_nums]

    vp = ax.violinplot(data, showmeans=False, showmedians=True)

    # Customize violin plot appearance
    for body in vp['bodies']:
        body.set_facecolor('lightblue')
        body.set_alpha(0.7)

    # Add mean markers
    means = [np.mean(latency_data[dev]['values']) for dev in dev_nums]
    ax.plot(range(1, len(dev_nums) + 1), means, 'rs', label='Mean')

    # Annotate mean values
    for i, mean in enumerate(means):
        ax.annotate(f'{mean:.1f}',
                   (i + 1, mean),
                   textcoords="offset points",
                   xytext=(0, 10),
                   ha='center')

    ax.set_xlabel('Device Index')
    ax.set_ylabel('Latency (milliseconds)')
    ax.set_title('BLE Mesh Network Latency by Device')
    ax.yaxis.grid(True)
    ax.legend()

    plt.tight_layout()
    return fig

def main():
    latency_data = parse_latency_data(log_data)

    fig = create_violinplot(latency_data)

    # Display summary statistics
    print("Latency Statistics Summary:")
    print("=" * 50)
    print(f"{'Dev':<5}{'Address':<10}{'Min':<8}{'Max':<8}{'Mean':<8}{'Median':<8}")
    print("-" * 50)

    for dev in sorted(latency_data.keys()):
        values = latency_data[dev]['values']
        print(f"{dev:<5}{latency_data[dev]['address']:<10}"
              f"{min(values):<8}{max(values):<8}"
              f"{np.mean(values):.1f}    {np.median(values):<8}")

    plt.savefig('mesh_latency_violinplot.png')
    print("\nPlot saved as 'mesh_latency_violinplot.png'")

if __name__ == "__main__":
    main()