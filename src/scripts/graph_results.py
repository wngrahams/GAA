from matplotlib import pyplot as plt
import numpy as np
import csv


def main():
    filenames = ["../../out/all.txt",
                 "../../out/island_1.txt",
                 "../../out/island_2.txt",
                 "../../out/island_3.txt",
                 "../../out/island_4.txt",
                 "../../out/island_5.txt"]
    colors = ['black', 'red', 'blue', 'green', 'pink', 'yellow']

    fig, ax = plt.subplots()
    for i in range(len(filenames)):
        y = []
        with open(filenames[i],'r') as csvfile:
            plots = csv.reader(csvfile)
            for row in plots:
                y.append(float(row[0]))
            x = np.arange(len(y))
            ax.plot(x, y, linewidth=0.6, color=colors[i])

    plt.grid(linestyle='--', linewidth=0.5)
    plt.xlabel('Generation')
    plt.minorticks_on()
    plt.ylabel('Fitness')
    plt.minorticks_on()
    plt.show()


if __name__ == '__main__':
    main()