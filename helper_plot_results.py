#!/usr/bin/env python3
# filepath: latency_analysis.py

import re
import numpy as np
import matplotlib.pyplot as plt

# Sample log data - this could  be read from a file in practice
log_data = """
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 0 addr 0x0001 avg latency: 159 ms failures 0 # values: 161 139 134 145 164 171 154 180 161 183 210 192 131 138 179 152 143 171 151 126
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 1 addr 0x0002 avg latency: 142 ms failures 0 # values: 168 160 160 131 149 134 135 135 145 119 140 153 137 132 168 121 115 201 119 131
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 2 addr 0x0003 avg latency: 123 ms failures 0 # values: 147 102 109 97 117 145 120 141 135 116 136 117 126 126 142 111 130 110 133 104
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 3 addr 0x0004 avg latency: 100 ms failures 0 # values: 98 94 100 89 94 109 101 113 95 100 87 91 142 84 97 98 83 125 117 91
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 4 addr 0x0005 avg latency:  88 ms failures 0 # values: 92 93 75 99 95 81 61 73 66 92 104 104 107 76 100 86 71 88 99 104
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 5 addr 0x0006 avg latency:  75 ms failures 0 # values: 62 102 63 73 115 54 67 66 77 107 80 58 77 65 67 125 57 54 82 68
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 6 addr 0x0007 avg latency:  55 ms failures 0 # values: 37 55 56 73 75 43 89 39 79 47 45 43 42 94 48 50 49 39 55 53
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 7 addr 0x0008 avg latency:  38 ms failures 0 # values: 61 28 29 42 56 25 64 40 23 35 34 33 37 48 32 27 43 28 32 43
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 8 addr 0x0009 avg latency:  15 ms failures 0 # values: 17 17 14 14 12 15 15 17 10 15 15 13 23 17 16 19 13 14 17 11
d_09: @00:04:16.049439  [00:04:16.049,438] <inf> mesh_nw_test: Dev 9 addr 0x000a avg latency:   0 ms failures 0 # values: 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
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

    vp = ax.violinplot(data, showmeans=False, showmedians=True, positions=dev_nums)

    # Customize violin plot appearance
    for body in vp['bodies']:
        body.set_facecolor('lightblue')
        body.set_alpha(0.7)

    # Add mean markers
    means = [np.mean(latency_data[dev]['values']) for dev in dev_nums]
    ax.plot(dev_nums, means, 'rs', label='Mean')

    # Annotate mean values
    for dev, mean in zip(dev_nums, means):
        ax.annotate(f'{mean:.1f}',
                   (dev, mean),
                   textcoords="offset points",
                   xytext=(0, 10),
                   ha='center')

    ax.set_xlabel('Device Number')
    ax.set_ylabel('Latency (milliseconds)')
    ax.set_title('BLE Mesh Network Latency by Device')
    ax.yaxis.grid(True)
    ax.set_xticks(dev_nums)
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