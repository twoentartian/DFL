import json
import networkx as nx
import pandas
import pandas as pd
import matplotlib.pyplot as plt
import tkinter
import colorsys
import scipy

config_file_path = 'simulator_config.json'
accuracy_file_path = 'accuracy.csv'
draw_from_tick = 10000
draw_per_tick = 1
draw_stop_tick = 15000
number_of_figure_per_row = 10

config_file = open(config_file_path)
config_file_content = config_file.read()
config_file_json = json.loads(config_file_content)
topology = config_file_json['node_topology']

accuracy_df = pandas.read_csv(accuracy_file_path, index_col=0, header=0)

print(accuracy_df)

total_tick = len(accuracy_df.index)


draw_counter = 0

tick_to_draw = []
for tick_index in range(total_tick):
    tick = accuracy_df.index[tick_index]
    if tick < draw_from_tick or tick > draw_stop_tick:
        continue

    if draw_counter == draw_per_tick:
        draw_counter = 0
    else:
        draw_counter = draw_counter + 1
        continue

    tick_to_draw.append(tick)

number_of_figure = len(tick_to_draw)
number_of_row = int(number_of_figure / 10) + 1

G = nx.Graph()
for singleItem in topology:
    unDirLink = singleItem.split('--')
    if len(unDirLink) != 1:
        G.add_edge(unDirLink[0], unDirLink[1])

    dirLink = singleItem.split('->')
    if len(dirLink) != 1:
        G.add_edge(dirLink[0], dirLink[1])

plt.ion()

#layout = nx.spring_layout(G)
#layout = nx.circular_layout(G)
#layout = nx.spectral_layout(G)
layout = nx.kamada_kawai_layout(G)

node_name = G.nodes
for tick in tick_to_draw:
    node_color = []
    node_labels = {}
    # node color
    for node in G.nodes:
        accuracy = accuracy_df.loc[tick, node]
        (r, g, b) = colorsys.hsv_to_rgb(accuracy, 1.0, 1.0)
        node_color.append([r, g, b])
        node_labels[node] = node + ":" + str(accuracy)

    plt.clf()
    plt.title("tick = " + str(tick))
    nx.draw(G, node_color=node_color, with_labels=True, pos=layout, font_color='k', labels=node_labels)
    plt.pause(0.1)
plt.ioff()
plt.show()

