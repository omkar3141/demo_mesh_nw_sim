# Bluetooth Mesh Network Simulation

This test demonstration shows how Babblsim test framework can be used for simulating Bluetooth Mesh networks. It allows testing different network topologies and evaluating mesh network functioning. The network topology itself is modeled by [NxN attenuation channel model](https://github.com/BabbleSim/ext_2G4_channel_multiatt) for configuring attenuation of each independent path between the nodes. The [BLE Simple modem](https://github.com/BabbleSim/ext_2G4_modem_BLE_simple/blob/master/docs/Description.pdf) is necessary for this test. This modem allows modelling of packet loss due to weak signals, basically isolating all the nodes which have high attenuation between them.

The attenuation for each path is provided by attenuation coefficients in the `.coeff` file. This demonstration also provides a script to create the attenuation coefficients and visualize the network topology.

## Test Organization

The test suite consists of several key components:

1. **Source Files**
   - `mesh_test.c`: Utilities and initialization functions
   - `mesh_nw_test.c`: Implementation of device and tester node behaviors
   - `test_scripts/test_network1.sh`: Test execution script for network topology 1. This script is provided as an example of how to run the test with a specific topology.
   - `test_scripts/test_1tester_ndevs_generic.sh`: Test execution script. This is a generic script that can be used to run the test with any number of devices and any topology. It takes the number of devices, the attenuation coefficients file, and the number of iterations as arguments.

2. **Network Configuration**
   - Custom topology definitions via node coordinates
   - Attenuation patterns between nodes stored in `.coeff` files
   - Visual network graphs generated as `.png` files, showing node positions and radio range connectivity. Nodes which are not in radio range are not shown as connected in the picture.

3. **Test Procedure**
   - Uses last instiated node as a tester and uses all nodes (including tester) as devices under test.
   - Tester starts with the first device.
     - Tester sends the [Health_Attention_Get](https://www.bluetooth.com/specifications/specs/html/?src=MshPRT_v1.1/out/en/index-en.html#UUID-fcfa6fca-6f36-ff67-fa0a-202ba5cd1622) message, and waits for response.
       - Note: Health client API is used in a synchronous (blocking) way, and therefore API waits until a response is received from the device.
     - Tester measures the time it took to receive a response message
     - If timeout occurs, error is recorded.
     - Procedure is repeated for `MAX_ITERATIONS` number of times.
   - Tester moves to next device and repeats above steps.
   - At the end, latency results are printed.

## Building and Running the Test

1. Check out v3.0.1 tag for nRF Connect SDK:
   ```bash
   cd /ncs/nrf
   git checkout v3.0.1
   west update
   ```

2. Clone this sample demonstration at `/ncs/zephyr` root directory
   ```bash
   cd /ncs/zephyr
   git clone https://github.com/omkar3141/demo_mesh_nw_sim
   ```

3. Compile and run the test. For example: To run the test with 10 devices, 20 message Get/Status transactions per device, and using the topology 1:
   ```bash
   cd /ncs/zephyr/demo_mesh_nw_sim
   ./compile.sh
   ./test_scripts/test_1tester_ndevs_generic.sh -n 10 -c network1_att_file.coeff -i 20
   ```

   Alternatively, you can also use `test_network1.sh` script to run the test with 10 devices and default number of iterations for topology 1:
   ```bash
   ./test_scripts/test_network1.sh
   ```

4. Test will run with example topology 1, and will print the results to the console.

Output will look like this:
```bash
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
d_09: @00:02:04.858796 INFO: test_node_tester PASSED
```

5. You can then visualize the test results by pasting the output in the helper script `helper_plot_results.py` and running it:
   ```bash
   python3 helper_plot_results.py
   ```

## Creating Network Topologies

### Network Topology Creation

Use the `helper_nw_att_file_creator.py` script to create new network topologies.

1. Define node positions
   Edit the script and define node coordinates. Examples:
   ```python
   # Linear chain topology
   nodes = [(0, 0), (1, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 6), (7, 7), (8, 8), (9, 9)]
   ```

2. Provide the file name in `fname` variable:
   ```python
   fname = "network1"  # Output filename
   ```

3. Provide the connectivity radius. This radius defines the maximum distance between nodes for them to be considered within the radio range. The script will linearly scale the attenuation (from 0 to `max_att_for_conn_radius`) against the distance between the nodes. Beyond this radius the attenuation will be set to 195 dBm, which effectively isolates the nodes.
   ```python
   connectivity_radius = 1.7  # Maximum connection distance
   ```

4. Provide the maximum attenuation for nodes within the connectivity radius:
   ```python
   max_att_for_conn_radius = 95  # Maximum attenuation for connected nodes
   ```

5. Generate network representation and attenuation coefficients file:
   ```bash
   python3 helper_nw_att_file_creator.py
   ```
   This creates:
   - `network1_att_file.coeff`: Attenuation coefficients
   - `network1.png`: Visual network diagram

6. Use `test_1_tester_n_dev_generic.sh` script to use the generated topology and run the test. The script takes the number of devices and the attenuation coefficients file as arguments. For example, to run the test with 10 devices using the generated coefficients file:
   ```bash
   ./test_scripts/test_1_tester_n_dev_generic.sh -n 10 -c network1_att_file.coeff
   ```

### Examples Topologies:

1. Example 1 - Linear chain topology
   ```python
   nodes = [(0, 0), (1, 1), (2, 2), (3, 3), (4, 4), (5, 5), (6, 6), (7, 7), (8, 8), (9, 9)]
   fname = "network1"
   connectivity_radius = 1.7
   max_att_for_conn_radius = 95
   ```

   Results in following topology:

   <img src="network1.png" alt="Network1 Topology" width="500"/>

   If you plot the results using `helper_plot_results.py`, the output violin plot will show the average round-trip latency for each device in the network. The x-axis will represent the device addresses, and the y-axis will represent the average latency in milliseconds. The plot will show the distribution of latencies for each device, allowing you to visualize the performance of the mesh network under the defined topology.

   The violin plot for this topology, with 20 messages exchanged for each node, looks like this:

   <img src="network1_violinplot.png" alt="Network1 Latency Plot" width="500"/>

2. Example 2
   ```python
   nodes = []
   for i in range(6):
       for j in range(4):
           nodes.append((i, j))
   fname = "network2"
   connectivity_radius = 1.7
   ```

   Results in following topology:

   <img src="network2.png" alt="Network2 Topology" width="500"/>

   The violin plot for this topology, with 20 messages exchanged for each node, looks like this:

   <img src="network2_violinplot.png" alt="Network2 Latency Plot" width="500"/>

3. Example 3
   ```python
   nodes = [(0, 0), (0, 1), (1, 0), (1, 1), (1.7, 2.5), (2.3, 1.5), (3, 3), (3, 4), (4, 3), (4, 4)]
   fname = "network3"
   connectivity_radius = 2.2
   max_att_for_conn_radius = 95
   ```

   Results in following topology:

   <img src="network3.png" alt="Network3 Topology" width="500"/>

   The violin plot for this topology, with 20 messages exchanged for each node, looks like this:

   <img src="network3_violinplot.png" alt="Network3 Latency Plot" width="500"/>


## Further exploration

You can build your own testing scenarios for specific applications and specific use-cases with this approch. It is recommended to test one specific thing at a time to keep test code simpler. However, you can construct as complex test as you want.

Refer to existing [Bluetooth Mesh Babblesim tests](https://github.com/zephyrproject-rtos/zephyr/tree/main/tests/bsim/bluetooth/mesh) for more inspiration. The existing tests are not
based on network topology, and they assume all devices can communicate with all other devices.

## Limitations

This networking simulation demonstration uses Zephyr's link layer for simplicity. Currently, it is not possible to use SoftDevice controller for Babblesim simulation.
However, as Bluetooth Mesh operates at higher protocol layer above the controller, such simulation can be a great tool to understand functioning of Bluetooth Mesh networking.

### Important Notes

- The simulation provides consistent, reproducible results with every execution. Even the "random" behavior in the mesh stack and controller's advertising bearer will produce identical outcomes across multiple runs.
- Combining this simulation with physical device testing provides a comprehensive validation approach, significantly enhancing the reliability of your application development process.
- When analyzing mesh message delivery to target nodes, use Unacknowledged Messages rather than Acknowledged Messages (which require responses). This approach provides clearer visibility into the message delivery process itself.
- For applications that require robustness of message exchange, you can implement custom application-level retry mechanisms to achieve complete reliability tailored to your specific use-cases.
