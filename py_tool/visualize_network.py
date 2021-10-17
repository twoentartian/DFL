import json
import networkx as nx
import pandas
import pandas as pd
import matplotlib.pyplot as plt
import tkinter
import colorsys
import os
import re

config_file_path = 'simulator_config.json'
accuracy_file_path = 'accuracy.csv'
peer_change_file_path = 'peer_change_record.txt'
draw_from_tick = 0
draw_per_tick = 1
draw_stop_tick = 20000

config_file = open(config_file_path)
config_file_content = config_file.read()
config_file_json = json.loads(config_file_content)
topology = config_file_json['node_topology']
peer_control_enabled = config_file_json['services']['peer_control_service']['enable']
nodes = config_file_json['nodes']

accuracy_df = pandas.read_csv(accuracy_file_path, index_col=0, header=0)

peer_change_file_exists = os.path.exists(peer_change_file_path)
peer_change_list = []
if peer_change_file_exists:
    peer_change_file = open(peer_change_file_path, "r+")
    peer_change_content = peer_change_file.readlines()
    for line in peer_change_content:
        operation = 0
        result = re.findall('tick:\d+', line)
        tick = int(result[0][5:])
        result = re.findall('\s\w*\(accuracy', line)
        lhs_node = result[0][1:-9]
        result = re.findall('\s\w*\(buffer', line)
        rhs_node = result[0][1:-7]
        add_ = re.findall('\sadd\s', line)
        delete_ = re.findall('\sdelete\s', line)
        if add_ and not delete_:
            operation = 1
        if not add_ and delete_:
            operation = 2
        peer_change_list.append({'tick':tick, 'lhs_node':lhs_node, 'rhs_node':rhs_node, 'operation':operation})

    peer_change_file.close()
print(accuracy_df)

total_tick = len(accuracy_df.index)


draw_counter = 0

tick_to_draw = []
for tick in accuracy_df.index:
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

for single_node in nodes:
    G.add_node(single_node['name'])

if not peer_control_enabled:
    for singleItem in topology:
        unDirLink = singleItem.split('--')
        if len(unDirLink) != 1:
            G.add_edge(unDirLink[0], unDirLink[1])

        dirLink = singleItem.split('->')
        if len(dirLink) != 1:
            G.add_edge(dirLink[0], dirLink[1])

plt.ion()

#layout = nx.spring_layout(G)
layout = nx.circular_layout(G)
#layout = nx.spectral_layout(G)
#layout = nx.kamada_kawai_layout(G)

node_name = G.nodes
peer_change_list_index = 0
plt.figure(figsize=[10, 10])
for tick in tick_to_draw:
    node_color = []
    node_labels = {}

    # node edge
    while peer_change_list_index < len(peer_change_list) and tick > peer_change_list[peer_change_list_index]['tick']:
        current_peer_change = peer_change_list[peer_change_list_index]
        if current_peer_change['operation'] == 1: # add edge
            G.add_edge(current_peer_change['lhs_node'], current_peer_change['rhs_node'])
        if current_peer_change['operation'] == 2: # delete edge
            G.remove_edge(current_peer_change['lhs_node'], current_peer_change['rhs_node'])
        peer_change_list_index = peer_change_list_index + 1

    # node color
    for node in G.nodes:
        accuracy = accuracy_df.loc[tick, node]
        (r, g, b) = colorsys.hsv_to_rgb(accuracy, 1.0, 1.0)
        node_color.append([r, g, b])
        node_labels[node] = node + ":" + str(accuracy)

    plt.clf()
    plt.title("tick = " + str(tick))
    nx.draw(G, node_color=node_color, with_labels=True, pos=layout, font_color='k', labels=node_labels)
    plt.pause(0.15)
plt.ioff()
plt.show()

