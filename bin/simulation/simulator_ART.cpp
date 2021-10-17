#include <random>
#include <iostream>
#include <mutex>

#include <glog/logging.h>
#include <configure_file.hpp>
#include <time_util.hpp>
#include <tmt.hpp>

class node_ART
{
public:
	node_ART()
	{
		next_train_tick = 0;
		model = 0.0;
		name = "empty";
		buffer_size = 0;
	}
	
	std::string name;
	std::vector<int> training_interval_tick;
	std::vector<node_ART*> peers;
	int next_train_tick;
	
	int buffer_size;
	
	std::vector<std::tuple<std::string, float>> parameter_buffer;
	std::mutex parameter_buffer_lock;
	
	float model;

};

std::unordered_map<std::string, node_ART*> node_container_ART;

int main(int argc, char *argv[])
{
	constexpr char config_file_path[] = "./simulator_config.json";
	
	//create new folder
	std::string time_str = time_util::time_to_text(time_util::get_current_utc_time());
	std::filesystem::path output_path = std::filesystem::current_path() / (time_str + "_ART");
	std::filesystem::create_directories(output_path);
	
	//log file path
	google::InitGoogleLogging(argv[0]);
	std::filesystem::path log_path(output_path / "log");
	if (!std::filesystem::exists(log_path)) std::filesystem::create_directories(log_path);
	google::SetLogDestination(google::INFO, (log_path.string() + "/").c_str());
	
	//load configuration
	configuration_file config;
	auto load_config_rc = config.LoadConfiguration(config_file_path);
	if (load_config_rc < 0)
	{
		LOG(FATAL) << "cannot load configuration file, wrong format?";
		return -1;
	}
	auto config_json = config.get_json();
	//backup configuration file
	std::filesystem::copy(config_file_path, output_path / "simulator_config.json");
	
	auto ml_max_tick = *config.get<int>("ml_max_tick");
	
	//load node configurations
	auto nodes_json = config_json["nodes"];
	size_t max_buffer_size = 0;
	for (auto &single_node: nodes_json)
	{
		const std::string node_name = single_node["name"];
		{
			auto iter = node_container_ART.find(node_name);
			if (iter != node_container_ART.end())
			{
				LOG(FATAL) << "duplicate node name";
				return -1;
			}
		}
		
		//name
		const int buf_size = single_node["buffer_size"];
		if (buf_size > max_buffer_size) max_buffer_size = buf_size;
		
		const std::string node_type = single_node["node_type"];
		
		auto* temp_node = new node_ART();
		temp_node->name = node_name;
		temp_node->next_train_tick = 0;
		temp_node->buffer_size = 0;
		
		auto[iter, status]=node_container_ART.emplace(node_name, temp_node);
		
		//training_interval_tick
		for (auto &el : single_node["training_interval_tick"])
		{
			iter->second->training_interval_tick.push_back(el);
		}
	}
	
	//load network topology configuration
	{
		const std::string fully_connect = "fully_connect";
		const std::string average_degree = "average_degree-";
		const std::string unidirectional_term = "->";
		const std::string bilateral_term = "--";
		
		auto node_topology_json = config_json["node_topology"];
		for (auto &topology_item : node_topology_json)
		{
			const std::string topology_item_str = topology_item.get<std::string>();
			auto average_degree_loc = topology_item_str.find(average_degree);
			auto unidirectional_loc = topology_item_str.find(unidirectional_term);
			auto bilateral_loc = topology_item_str.find(bilateral_term);
			
			auto check_duplicate_peer = [](const node_ART &target_node, const std::string &peer_name) -> bool
			{
				bool duplicate = false;
				for (const auto &current_connections: target_node.peers)
				{
					if (peer_name == current_connections->name)
					{
						duplicate = true;
						break;
					}
				}
				return duplicate;
			};
			
			if (topology_item_str == fully_connect)
			{
				LOG(INFO) << "network topology is fully connect";
				
				for (auto&[node_name, node_inst] : node_container_ART)
				{
					for (auto&[target_node_name, target_node_inst] : node_container_ART)
					{
						if (node_name != target_node_name)
						{
							node_inst->peers.push_back(target_node_inst);
						}
					}
				}
				break;
			}
			else if (average_degree_loc != std::string::npos)
			{
				std::string degree_str = topology_item_str.substr(average_degree_loc + average_degree.length());
				int degree = std::stoi(degree_str);
				LOG(INFO) << "network topology is average degree: " << degree;
				LOG_IF(FATAL, degree > node_container_ART.size() - 1) << "degree > node_count - 1, impossible to reach such large degree";
				
				std::vector<std::string> node_name_list;
				node_name_list.reserve(node_container_ART.size());
				for (const auto&[node_name, node_inst] : node_container_ART)
				{
					node_name_list.push_back(node_name);
				}
				
				std::random_device dev;
				std::mt19937 rng(dev());
				LOG(INFO) << "network topology average degree process begins";
				for (auto&[target_node_name, target_node_inst] : node_container_ART)
				{
					std::shuffle(std::begin(node_name_list), std::end(node_name_list), rng);
					for (const std::string &connect_node_name : node_name_list)
					{
						//check average degree first to ensure there are already some connects.
						if (target_node_inst->peers.size() >= degree) break;
						if (connect_node_name == target_node_name) continue; // connect_node == self
						if (check_duplicate_peer((*target_node_inst), connect_node_name)) continue; // connection already exists
						
						auto &connect_node = node_container_ART.at(connect_node_name);
						target_node_inst->peers.push_back(connect_node);
						LOG(INFO) << "network topology average degree process: " << target_node_inst->name << " -> " << connect_node->name;
					}
				}
				LOG(INFO) << "network topology average degree process ends";
			}
			else if (unidirectional_loc != std::string::npos)
			{
				std::string lhs_node_str = topology_item_str.substr(0, unidirectional_loc);
				std::string rhs_node_str = topology_item_str.substr(unidirectional_loc + unidirectional_term.length());
				LOG(INFO) << "network topology: unidirectional connect " << lhs_node_str << " to " << rhs_node_str;
				auto lhs_node_iter = node_container_ART.find(lhs_node_str);
				auto rhs_node_iter = node_container_ART.find(rhs_node_str);
				LOG_IF(FATAL, lhs_node_iter == node_container_ART.end()) << lhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				LOG_IF(FATAL, rhs_node_iter == node_container_ART.end()) << rhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				
				if (check_duplicate_peer(*(lhs_node_iter->second), rhs_node_str))// connection already exists
				{
					LOG(WARNING) << rhs_node_str << " is already a peer of " << lhs_node_str;
				}
				else
				{
					auto &connect_node = node_container_ART.at(rhs_node_str);
					lhs_node_iter->second->peers.push_back(connect_node);
				}
			}
			else if (bilateral_loc != std::string::npos)
			{
				std::string lhs_node_str = topology_item_str.substr(0, bilateral_loc);
				std::string rhs_node_str = topology_item_str.substr(bilateral_loc + unidirectional_term.length());
				LOG(INFO) << "network topology: bilateral connect " << lhs_node_str << " with " << rhs_node_str;
				auto lhs_node_iter = node_container_ART.find(lhs_node_str);
				auto rhs_node_iter = node_container_ART.find(rhs_node_str);
				LOG_IF(FATAL, lhs_node_iter == node_container_ART.end()) << lhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				LOG_IF(FATAL, rhs_node_iter == node_container_ART.end()) << rhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				
				if (check_duplicate_peer(*(lhs_node_iter->second), rhs_node_str))// connection already exists
				{
					LOG(WARNING) << rhs_node_str << " is already a peer of " << lhs_node_str;
				}
				else
				{
					auto &rhs_connected_node = node_container_ART.at(rhs_node_str);
					lhs_node_iter->second->peers.push_back(rhs_connected_node);
				}
				
				if (check_duplicate_peer(*(rhs_node_iter->second), lhs_node_str))// connection already exists
				{
					LOG(WARNING) << lhs_node_str << " is already a peer of " << rhs_node_str;
				}
				else
				{
					auto &lhs_connected_node = node_container_ART.at(lhs_node_str);
					rhs_node_iter->second->peers.push_back(lhs_connected_node);
				}
			}
			else
			{
				LOG(ERROR) << "unknown topology item: " << topology_item_str;
			}
		}
	}
	
	//node vector container
	std::vector<node_ART*> node_vector_container_ART;
	node_vector_container_ART.reserve(node_container_ART.size());
	for (auto &single_node : node_container_ART)
	{
		node_vector_container_ART.push_back(single_node.second);
	}
	
	int tick = 0;
	while (tick <= ml_max_tick)
	{
		std::cout << "tick: " << tick << " (" << ml_max_tick << ")" << std::endl;
		LOG(INFO) << "tick: " << tick << " (" << ml_max_tick << ")";
		bool exit = false;
		
		tmt::ParallelExecution_StepIncremental([&tick](uint32_t index, uint32_t thread_index, node_ART* single_node){
			if (tick >= single_node->next_train_tick)
			{
				std::random_device dev;
				std::mt19937 rng(dev());
				std::uniform_int_distribution<int> distribution(0, int(single_node->training_interval_tick.size()) - 1);
				single_node->next_train_tick += single_node->training_interval_tick[distribution(rng)];
				
				////TODO:train
				std::normal_distribution<float> model_train_distribution(0.01, 0.005);
				single_node->model = single_node->model + model_train_distribution(rng);
				if (single_node->model < 0) single_node->model = 0;
				if (single_node->model > 1) single_node->model = 1;
				
				//add ML network to FedAvg buffer
				for (auto updating_node : single_node->peers)
				{
					std::lock_guard guard(updating_node->parameter_buffer_lock);
					updating_node->parameter_buffer.emplace_back(single_node->name, single_node->model);
				}
				
			}
		}, node_vector_container_ART.size(), node_vector_container_ART.data());
		
		//check fedavg buffer full
		tmt::ParallelExecution_StepIncremental([&tick](uint32_t index, uint32_t thread_index, node_ART* single_node){
			if (single_node->parameter_buffer.size() >= single_node->buffer_size)
			{
				////TODO:update model
				
				
				//clear buffer and start new loop
				single_node->parameter_buffer.clear();
			}
		}, node_vector_container_ART.size(), node_vector_container_ART.data());
	}
	
	return 0;
}