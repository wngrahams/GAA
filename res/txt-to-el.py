from pathlib import Path
import sys

node_dict = dict()
edge_list = []
node_cnt = 0
edge_cnt = 0

infile = sys.argv[1]
with open(infile, "r") as i_f:

    for line in i_f:
        if line[0] != "#" and line[0] != "/" and line[0] != "\n":  # ignore comments and blank lines

            split_line = line.split()

            if split_line[0] not in node_dict:
                node_dict[split_line[0]] = node_cnt
                node_cnt += 1

            if split_line[1] not in node_dict:
                node_dict[split_line[1]] = node_cnt
                node_cnt += 1

            # not adding edges between the same node
            if node_dict[split_line[0]] != node_dict[split_line[1]]:
                edge_cnt += 1
                edge_list.append((node_dict[split_line[0]], node_dict[split_line[1]]))


outfile = Path(infile).stem + ".edgelist"
with open(outfile, "w") as o_f:
    o_f.write("|v|: " + str(node_cnt) + "\n")
    o_f.write("|e|: " + str(edge_cnt) + "\n")

    for edge in edge_list:
        o_f.write(str(edge[0]) + " " + str(edge[1]) + "\n")



