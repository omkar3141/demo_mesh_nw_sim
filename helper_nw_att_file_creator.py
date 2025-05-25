import math
import networkx as nx
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors

# Select/Create topology:
# -----------------------

# Topology 1
# Creating 30 nodes in linear topology
# nodes = []
# for i in range(10):
#         nodes.append((i, i))
# fname = "network1"
# connectivity_radius = 1.7

# Topology 2
# Creating 24 nodes in a 4x6 grid topology
nodes = []
for i in range(6):
    for j in range(4):
        nodes.append((i, j))
fname = "network2"
connectivity_radius = 2.9

# Topology 3
# nodes = [(0, 0), (0, 1), (1, 0), (1, 1), (1.7, 2.5), (2.3, 1.5), (3, 3), (3, 4), (4, 3), (4, 4)]
# fname = "network3"
# connectivity_radius = 2.2

# --------------------------------------------------------------------------------------------------

# Attenuation beyond this will not be added in the attenuation file, and instead
# default attenuation of 150 dbm will be used.
max_att_for_conn_radius = 95


def calculate_attenuation(node1, node2, max_distance=connectivity_radius,
                          max_att=max_att_for_conn_radius,
                          def_att=max_att_for_conn_radius+100):
    x1, y1 = node1
    x2, y2 = node2
    distance = math.sqrt((x2 - x1)**2 + (y2 - y1)**2)

    # linear scaling from 0 to -150 dBm, from 0 to 5 meters
    if distance > max_distance:
        return def_att

    attenuation = max_att * (distance / max_distance)
    return attenuation


def generate_attenuation_file(nodes, fname, max_att=max_att_for_conn_radius):
    with open(fname + '_att_file.coeff', 'w') as f:
        for i in range(len(nodes)):
            for j in range(i+1, len(nodes)):
                node1 = nodes[i]
                node2 = nodes[j]
                attenuation = calculate_attenuation(node1, node2)

                # Write both directions
                line1 = f"{i} {j}: {attenuation:.2f} # Node{i} to Node{j} attenuation in dBm\n"
                line2 = f"{j} {i}: {attenuation:.2f} # Node{j} to Node{i} attenuation in dBm\n"
                f.write(line1)
                f.write(line2)
                print(line1, end='')
                print(line2, end='')


def visualize_network(nodes, max_distance=connectivity_radius):
    G = nx.Graph()

    # Get maximum network size in x and y directions
    x_size = 0
    y_size = 0
    for n in nodes:
        x, y = n
        x_size = max(x_size, x)
        y_size = max(y_size, y)

    plt.figure(figsize=(x_size, y_size), dpi=150)

    for i, node in enumerate(nodes):
        G.add_node(i, pos=node)

    for i in range(len(nodes)):
        for j in range(i+1, len(nodes)):
            node1 = nodes[i]
            node2 = nodes[j]
            G.add_edge(i, j)

    pos = nx.get_node_attributes(G, 'pos')
    edges = G.edges()

    # Create a colormap for the line colors
    cmap = mcolors.LinearSegmentedColormap.from_list("", ["#ff0000", "#ff6666", "#ff9999", "#ffc5c5"])

    for edge in edges:
        node1 = pos[edge[0]]
        node2 = pos[edge[1]]
        distance = math.sqrt((node2[0] - node1[0])**2 + (node2[1] - node1[1])**2)

        if distance <= 1:
            color = cmap(0.1)
            nx.draw_networkx_edges(G, pos, edgelist=[edge], edge_color=[color], width=3, alpha=0.9)
        elif distance <= max_distance:
            color = cmap(distance / max_distance)
            nx.draw_networkx_edges(G, pos, edgelist=[edge], edge_color=[color], width=3, alpha=0.9)

    nx.draw_networkx_nodes(G, pos, node_size=450)
    labels = {i: 'D'+str(i) for i in G.nodes()}
    nx.draw_networkx_labels(G, pos, labels=labels)
    plt.savefig(fname + ".png")


generate_attenuation_file(nodes, fname)
visualize_network(nodes)