import networkx as nx
from pathlib import Path
import sys

filename = sys.argv[1]

G = nx.Graph()

filetype = Path(filename).suffix

# import file into Graph object
if filetype == ".edgelist":
    with open(filename, "r") as f:

        line_cnt = 0
        for line in f:
            if line_cnt >= 2:
                split_line = line.split()
                G.add_edge(int(split_line[0]), int(split_line[1]), weight=1)

            line_cnt += 1


print("Number of nodes: " + str(G.number_of_nodes()))
#print(list(G.nodes))
print("Number of edges: " + str(G.number_of_edges()))
#print(list(G.edges))
print()

# run the Kernighan-Lin bisection algorithm:
partition = nx.algorithms.community.kernighan_lin.kernighan_lin_bisection(G, max_iter=1)
#print(partition[0])
#print(partition[1])
#print()

fitness = 0
# calculate fitness:
for edge in G.edges:
    # for each edge in the graph, add the edge weight to the fitness if the two nodes are in different partitions:
    if (edge[0] in partition[0] and edge[1] in partition[1]) or (edge[0] in partition[1] and edge[1] in partition[0]):
        # print(edge[0])
        # print(edge[1])
        # print(G[edge[0]][edge[1]]['weight'])
        fitness += G[edge[0]][edge[1]]['weight']

p0_weight = 0
p1_weight = 0
# add abs|(sum of node weights in partition 0) - (sum of node weights in partition 1)|
# currently assumming all node weights are 1
for node in G.nodes:

    if node in partition[0]:
        p0_weight += 1
    elif node in partition [1]:
        p1_weight += 1
    else:
        print("ERROR")
        exit(1)

fitness += abs(p0_weight - p1_weight)

print("Fitness = " + str(fitness))
