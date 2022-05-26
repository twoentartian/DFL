//
// Created by tyd, 17-Sep-21.
//

#include <iostream>
#include <algorithm>
#include <random>
#include <set>
#include <glog/logging.h>
#include <configure_file.hpp>

configuration_file::json get_default_simulation_configuration()
{
	configuration_file::json output;
	
	output["node_count"] = 100;
	output["buffer_size"] = 100;
	output["dataset_mode"] = "iid";
	output["model_generation_type"] = "normal";
	output["filter_limit"] = 0.5;
	output["training_interval_tick"] = configuration_file::json::array({8,9,10,11,12});
	
	output["node_peer_connection_count"] = 8;
	output["node_peer_connection_type"] = "--";
	configuration_file::json malicious_node;
	malicious_node["malicious_random_strategy"] = 1;
	malicious_node["malicious_strategy_1"] = 1;
	output["special_node"] = malicious_node;
	return output;
}

int main(int argc, char* argv[])
{
	configuration_file my_config;
	my_config.SetDefaultConfiguration(get_default_simulation_configuration());
	auto load_config_rc = my_config.LoadConfiguration("large_scale_config.json");
	if (load_config_rc < 0)
	{
		LOG(FATAL) << "cannot load large scale configuration file, wrong format?";
		return -1;
	}
	auto my_config_json = my_config.get_json();
	
	int node_count = *my_config.get<int>("node_count");
	int buffer_size = *my_config.get<int>("buffer_size");
	std::string dataset_mode = *my_config.get<std::string>("dataset_mode");
	std::string model_generation_type = *my_config.get<std::string>("model_generation_type");
	float filter_limit = *my_config.get<float>("filter_limit");
	int node_peer_connection_count = *my_config.get<int>("node_peer_connection_count");
	std::string node_peer_connection_type = *my_config.get<std::string>("node_peer_connection_type");
	auto special_node = my_config_json["special_node"];
	std::cout << "node_count: " << node_count << std::endl;
	std::cout << "buffer_size: " << buffer_size << std::endl;
	std::cout << "dataset_mode: " << dataset_mode << std::endl;
	std::cout << "model_generation_type: " << model_generation_type << std::endl;
	std::cout << "filter_limit: " << filter_limit << std::endl;
	std::cout << "node_peer_connection_count: " << node_peer_connection_count << std::endl;
	
	if (node_peer_connection_count % 2 != 0 && node_peer_connection_type == "--")
	{
		std::cout << "warning: node_peer_connection_count must be an even number with " << node_peer_connection_type << " connection."<< std::endl;
		return -1;
	}
	
	if (node_peer_connection_type != "--" && node_peer_connection_type != "->")
	{
		std::cout << "unknown node_peer_connection_type: " << node_peer_connection_type << std::endl;
		return -1;
	}
	
	{
		////set nodes
		configuration_file config;
		std::string config_file;
		
		if (argc == 2)
		{
			config_file.assign(argv[1]);
		}
		else if (argc == 1)
		{
			config_file.assign("../simulation/simulator_config.json");
		}
		else
		{
			std::cout << "large_scale_simulation_generator [config_file_path]" << std::endl;
			std::cout << "large_scale_simulation_generator" << std::endl;
			return -1;
		}
		
		std::filesystem::path config_file_path(config_file);
		if (!std::filesystem::exists(config_file_path))
		{
			std::cerr << config_file << " does not exist" << std::endl;
			return -1;
		}
		
		config.LoadConfiguration(config_file);
		auto& config_json = config.get_json();
		configuration_file::json& nodes_json = config.get_json()["nodes"];
		nodes_json.clear();
		for (int i = 0; i < node_count; ++i)
		{
			configuration_file::json node;
			node["name"] = std::to_string(i);
			node["dataset_mode"] = dataset_mode;
			node["training_interval_tick"] = my_config_json["training_interval_tick"] ;
			node["buffer_size"] = buffer_size;
			node["model_generation_type"] = model_generation_type;
			node["filter_limit"] = filter_limit;
			node["node_type"] = "normal";
			configuration_file::json node_non_iid = configuration_file::json::object();
			node_non_iid["1"] = configuration_file::json::array({1.0,2.0});
			node_non_iid["3"] = configuration_file::json::array({2.0,3.0});
			node["non_iid_distribution"] = node_non_iid;
			nodes_json.push_back(node);
		}
		
		//add malicious nodes
		int node_index = 0;
		for (auto& [node_type, count] : special_node.items())
		{
			int node_index_per_type = 0;
			while (node_index_per_type < count)
			{
				nodes_json[node_index]["node_type"] = node_type;
				std::cout << "set node " << node_index << " to special: " << node_type << std::endl;
				node_index_per_type++;
				node_index++;
			}
		}
		
		////set node topology
		std::vector<int> node_list_ex_self;
		node_list_ex_self.resize(node_count-1);
		for (int i = 0; i < node_list_ex_self.size(); ++i)
		{
			node_list_ex_self[i] = i;
		}
		std::vector<std::vector<int>> node_connections;
		node_connections.resize(node_count);
		std::random_device rd;
		std::mt19937 g(rd());
		for (int i = 0; i < node_count; ++i)
		{
			std::shuffle(node_list_ex_self.begin(), node_list_ex_self.end(), g);
			int count = node_peer_connection_count;
			auto iter = node_list_ex_self.begin();
			while (count --)
			{
				node_connections[i].push_back(*iter);
				iter++;
			}
			
			for (auto &peer : node_connections[i])
			{
				if (peer >= i)
				{
					peer++;
				}
			}
		}
		
		auto& node_topology_json = config_json["node_topology"];
		node_topology_json.clear();
		for (int i = 0; i < node_count; ++i)
		{
			std::cout << std::to_string(i) << node_peer_connection_type << "{";
			if (node_peer_connection_type == "--")
			{
				for (int j = 0; j < node_peer_connection_count/2; ++j)
				{
					std::cout << std::to_string(node_connections[i][j]) << ", ";
					node_topology_json.push_back(std::to_string(i) + node_peer_connection_type + std::to_string(node_connections[i][j]));
				}
			}
			else
			{
				for (int j = 0; j < node_peer_connection_count; ++j)
				{
					std::cout << std::to_string(node_connections[i][j]) << ", ";
					node_topology_json.push_back(std::to_string(i) + node_peer_connection_type + std::to_string(node_connections[i][j]));
				}
			}
			std::cout << "}" << std::endl;

		}
		
		config_json["ml_delayed_test_accuracy"] = false;
		
		config.write_back();
	}
	
	return 0;
}
