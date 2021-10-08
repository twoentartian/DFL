//
// Originally created by tyd, the malicious part is by jxzhang on 09-08-21.
//

#include <thread>
#include <unordered_map>
#include <fstream>
#include <set>
#include <atomic>
#include <chrono>

#include <glog/logging.h>

#include <tmt.hpp>
#include <configure_file.hpp>
#include <crypto.hpp>
#include <auto_multi_thread.hpp>
#include <util.hpp>
#include <time_util.hpp>
#include <ml_layer.hpp>
#include <thread_pool.hpp>
#include <dll_importer.hpp>
#include <boost/format.hpp>
#include <utility>

#include "../reputation_sdk.hpp"
#include "./default_simulation_config.hpp"
#include "./simulation_util.hpp"
#include "./node.hpp"
#include "./simulation_records.hpp"

/** assumptions in this simulator:
 *  (1) no transaction transmission time
 *  (2) no blockchain overhead
 *
 */

using model_datatype = float;

//return: train_data,train_label
std::tuple<std::vector<Ml::tensor_blob_like<model_datatype>>, std::vector<Ml::tensor_blob_like<model_datatype>>>
get_dataset_by_node_type(Ml::data_converter<model_datatype> &dataset, const node<model_datatype> &target_node, int size, const std::vector<int> &ml_dataset_all_possible_labels)
{
	Ml::tensor_blob_like<model_datatype> label;
	label.getShape() = {1};
	std::vector<Ml::tensor_blob_like<model_datatype>> train_data, train_label;
	if (target_node.dataset_mode == dataset_mode_type::default_dataset)
	{
		//iid dataset
		std::tie(train_data, train_label) = dataset.get_random_data(size);
	}
	else if (target_node.dataset_mode == dataset_mode_type::iid_dataset)
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<int> distribution(0, int(ml_dataset_all_possible_labels.size()) - 1);
		for (int i = 0; i < size; ++i)
		{
			int label_int = ml_dataset_all_possible_labels[distribution(rng)];
			label.getData() = {model_datatype(label_int)};
			auto[train_data_slice, train_label_slice] = dataset.get_random_data_by_Label(label, 1);
			train_data.insert(train_data.end(), train_data_slice.begin(), train_data_slice.end());
			train_label.insert(train_label.end(), train_label_slice.begin(), train_label_slice.end());
		}
	}
	else if (target_node.dataset_mode == dataset_mode_type::non_iid_dataset)
	{
		//non-iid dataset
		std::random_device dev;
		std::mt19937 rng(dev());
		
		Ml::non_iid_distribution<model_datatype> label_distribution;
		for (auto &target_label : ml_dataset_all_possible_labels)
		{
			label.getData() = {model_datatype(target_label)};
			auto iter = target_node.special_non_iid_distribution.find(target_label);
			if (iter != target_node.special_non_iid_distribution.end())
			{
				auto[dis_min, dis_max] = iter->second;
				if (dis_min == dis_max)
				{
					label_distribution.add_distribution(label, dis_min);
				}
				else
				{
					std::uniform_real_distribution<model_datatype> distribution(dis_min, dis_max);
					label_distribution.add_distribution(label, distribution(rng));
				}
			}
			else
			{
				LOG(ERROR) << "cannot find the desired label";
			}
		}
		std::tie(train_data, train_label) = dataset.get_random_non_iid_dataset(label_distribution, size);
	}
	return {train_data, train_label};
}

std::unordered_map<std::string, node<model_datatype> *> node_container;
dll_loader<reputation_interface<model_datatype>> reputation_dll;


int main(int argc, char *argv[])
{
	constexpr char config_file_path[] = "./simulator_config.json";
	
	//register node types
	normal_node<model_datatype>::registerNodeType();
	observer_node<model_datatype>::registerNodeType();
	malicious_model_poisoning_random_model_node<model_datatype>::registerNodeType();
	malicious_model_poisoning_random_model_by_turn_node<model_datatype>::registerNodeType();
	malicious_model_poisoning_random_model_biased_0_1_node<model_datatype>::registerNodeType();
	malicious_duplication_attack_node<model_datatype>::registerNodeType();
	malicious_data_poisoning_shuffle_label_node<model_datatype>::registerNodeType();
	malicious_data_poisoning_shuffle_label_biased_1_node<model_datatype>::registerNodeType();
	malicious_data_poisoning_random_data_node<model_datatype>::registerNodeType();
	
	//create new folder
	std::string time_str = time_util::time_to_text(time_util::get_current_utc_time());
	std::filesystem::path output_path = std::filesystem::current_path() / time_str;
	std::filesystem::create_directories(output_path);
	
	//log file path
	google::InitGoogleLogging(argv[0]);
	std::filesystem::path log_path(output_path / "log");
	if (!std::filesystem::exists(log_path)) std::filesystem::create_directories(log_path);
	google::SetLogDestination(google::INFO, (log_path.string() + "/").c_str());
	
	//reputation folder
	std::filesystem::path reputation_folder = output_path / "reputation";
	if (!std::filesystem::exists(reputation_folder)) std::filesystem::create_directories(reputation_folder);
	
	//load configuration
	configuration_file config;
	config.SetDefaultConfiguration(get_default_simulation_configuration());
	auto load_config_rc = config.LoadConfiguration(config_file_path);
	if (load_config_rc < 0)
	{
		LOG(FATAL) << "cannot load configuration file, wrong format?";
		return -1;
	}
	auto config_json = config.get_json();
	//backup configuration file
	std::filesystem::copy(config_file_path, output_path / "simulator_config.json");
	
	//update global var
	auto ml_solver_proto = *config.get<std::string>("ml_solver_proto");
	auto ml_train_dataset = *config.get<std::string>("ml_train_dataset");
	auto ml_train_dataset_label = *config.get<std::string>("ml_train_dataset_label");
	auto ml_test_dataset = *config.get<std::string>("ml_test_dataset");
	auto ml_test_dataset_label = *config.get<std::string>("ml_test_dataset_label");
	
	auto ml_max_tick = *config.get<int>("ml_max_tick");
	auto ml_train_batch_size = *config.get<int>("ml_train_batch_size");
	auto ml_test_interval_tick = *config.get<int>("ml_test_interval_tick");
	auto ml_test_batch_size = *config.get<int>("ml_test_batch_size");
	auto ml_model_weight_diff_record_interval_tick = *config.get<int>("ml_model_weight_diff_record_interval_tick");
	
	auto report_time_remaining_per_tick_elapsed = *config.get<int>("report_time_remaining_per_tick_elapsed");
	
	std::vector<int> ml_dataset_all_possible_labels = *config.get_vec<int>("ml_dataset_all_possible_labels");
	std::vector<float> ml_non_iid_normal_weight = *config.get_vec<float>("ml_non_iid_normal_weight");
	LOG_IF(ERROR, ml_non_iid_normal_weight.size() != 2) << "ml_non_iid_normal_weight must be a two-value array, {max min}";
	auto ml_reputation_dll_path = *config.get<std::string>("ml_reputation_dll_path");
	
	//load reputation dll
	if constexpr(std::is_same_v<model_datatype, float>)
	{
		auto[status, msg] = reputation_dll.load(ml_reputation_dll_path, export_class_name_reputation_float);
		LOG_IF(FATAL, !status) << "error to load reputation dll: " << msg;
	}
	else if constexpr(std::is_same_v<model_datatype, double>)
	{
		auto[status, msg] = reputation_dll.load(ml_reputation_dll_path, export_class_name_reputation_double);
		LOG_IF(FATAL, !status) << "error to load reputation dll: " << msg;
	}
	else
	{
		LOG(FATAL) << "unknown model datatype";
		return -1;
	}
	//backup reputation file
	{
		std::filesystem::path ml_reputation_dll(ml_reputation_dll_path);
		std::filesystem::copy(ml_reputation_dll_path, output_path / ml_reputation_dll.filename());
	}
	
	//load node configurations
	auto nodes_json = config_json["nodes"];
	size_t max_buffer_size = 0;
	for (auto &single_node: nodes_json)
	{
		const std::string node_name = single_node["name"];
		{
			auto iter = node_container.find(node_name);
			if (iter != node_container.end())
			{
				LOG(FATAL) << "duplicate node name";
				return -1;
			}
		}
		
		//name
		const int buf_size = single_node["buffer_size"];
		if (buf_size > max_buffer_size) max_buffer_size = buf_size;
		
		const std::string node_type = single_node["node_type"];
		
		node<model_datatype> *temp_node = nullptr;
		
		//find the node in the registered node map
		{
			auto result = node<model_datatype>::get_node_by_type(node_type);
			if (result == nullptr)
			{
				LOG(FATAL) << "unknown node type:" << node_type;
			}
			temp_node = result->new_node(node_name, buf_size);
		}
		
		auto[iter, status]=node_container.emplace(node_name, temp_node);
		
		//load models solver and open reputation_fileS
		iter->second->solver->load_caffe_model(ml_solver_proto);
		iter->second->open_reputation_file(reputation_folder);
		
		//dataset mode
		const std::string dataset_mode_str = single_node["dataset_mode"];
		if (dataset_mode_str == "default")
		{
			iter->second->dataset_mode = dataset_mode_type::default_dataset;
		}
		else if (dataset_mode_str == "iid")
		{
			iter->second->dataset_mode = dataset_mode_type::iid_dataset;
		}
		else if (dataset_mode_str == "non-iid")
		{
			iter->second->dataset_mode = dataset_mode_type::non_iid_dataset;
		}
		else
		{
			LOG(FATAL) << "unknown dataset_mode:" << dataset_mode_str;
			return -1;
		}
		
		//model_generation_type
		const std::string model_generation_type_str = single_node["model_generation_type"];
		if (model_generation_type_str == "compressed")
		{
			iter->second->model_generation_type = Ml::model_compress_type::compressed_by_diff;
		}
		else if (model_generation_type_str == "normal")
		{
			iter->second->model_generation_type = Ml::model_compress_type::normal;
		}
		else
		{
			LOG(FATAL) << "unknown model_generation_type:" << model_generation_type_str;
			return -1;
		}
		
		//filter_limit
		iter->second->filter_limit = single_node["filter_limit"];
		
		//label_distribution
		std::string dataset_mode = single_node["dataset_mode"];
		if (dataset_mode == "iid")
		{
			//nothing to do because the single_node["non_iid_distribution"] will not be used
		}
		else if (dataset_mode == "non-iid")
		{
			configuration_file::json non_iid_distribution = single_node["non_iid_distribution"];
			for (auto non_iid_item = non_iid_distribution.begin(); non_iid_item != non_iid_distribution.end(); ++non_iid_item)
			{
				int label = std::stoi(non_iid_item.key());
				auto min_max_array = *non_iid_item;
				float min = min_max_array.at(0);
				float max = min_max_array.at(1);
				if (max > min)
				{
					iter->second->special_non_iid_distribution[label] = {min, max};
				}
				else
				{
					iter->second->special_non_iid_distribution[label] = {max, min}; //swap the order
				}
			}
			for (auto &el: ml_dataset_all_possible_labels)
			{
				auto iter_el = iter->second->special_non_iid_distribution.find(el);
				if (iter_el == iter->second->special_non_iid_distribution.end())
				{
					//not set before
					iter->second->special_non_iid_distribution[el] = {ml_non_iid_normal_weight[0], ml_non_iid_normal_weight[1]};
				}
			}
		}
		else if (dataset_mode == "default")
		{
			//nothing to do because the single_node["non_iid_distribution"] will not be used
		}
		else
		{
			LOG(ERROR) << "unknown dataset_mode:" << single_node["dataset_mode"];
		}
		
		//training_interval_tick
		for (auto &el : single_node["training_interval_tick"])
		{
			iter->second->training_interval_tick.push_back(el);
		}
	}
	
	//load network topology configuration
	/** network topology configuration
	 * you can use fully_connect, average_degree-{degree}, 1->2, 1<->2, the topology items' order in the configuration file determines the order of adding connections.
	 * fully_connect: connect all nodes, and ignore all other topology items.
	 * average_degree-: connect the network to reach the degree for all nodes. If there are previous added topology, average_degree will add more connection
	 * 					to reach the degree and no duplicate connections.
	 * 1->2: add 2 as the peer of 1.
	 * 1--2: add 2 as the peer of 1 and 1 as the peer of 2.
	 */
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
			
			auto check_duplicate_peer = [](const node<model_datatype> &target_node, const std::string &peer_name) -> bool
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
				
				for (auto&[node_name, node_inst] : node_container)
				{
					for (auto&[target_node_name, target_node_inst] : node_container)
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
				LOG_IF(FATAL, degree > node_container.size() - 1) << "degree > node_count - 1, impossible to reach such large degree";
				
				std::vector<std::string> node_name_list;
				node_name_list.reserve(node_container.size());
				for (const auto&[node_name, node_inst] : node_container)
				{
					node_name_list.push_back(node_name);
				}
				
				std::random_device dev;
				std::mt19937 rng(dev());
				LOG(INFO) << "network topology average degree process begins";
				for (auto&[target_node_name, target_node_inst] : node_container)
				{
					std::shuffle(std::begin(node_name_list), std::end(node_name_list), rng);
					for (const std::string &connect_node_name : node_name_list)
					{
						//check average degree first to ensure there are already some connects.
						if (target_node_inst->peers.size() >= degree) break;
						if (connect_node_name == target_node_name) continue; // connect_node == self
						if (check_duplicate_peer((*target_node_inst), connect_node_name)) continue; // connection already exists
						
						auto &connect_node = node_container.at(connect_node_name);
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
				auto lhs_node_iter = node_container.find(lhs_node_str);
				auto rhs_node_iter = node_container.find(rhs_node_str);
				LOG_IF(FATAL, lhs_node_iter == node_container.end()) << lhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				LOG_IF(FATAL, rhs_node_iter == node_container.end()) << rhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				
				if (check_duplicate_peer(*(lhs_node_iter->second), rhs_node_str))// connection already exists
				{
					LOG(WARNING) << rhs_node_str << " is already a peer of " << lhs_node_str;
				}
				else
				{
					auto &connect_node = node_container.at(rhs_node_str);
					lhs_node_iter->second->peers.push_back(connect_node);
				}
			}
			else if (bilateral_loc != std::string::npos)
			{
				std::string lhs_node_str = topology_item_str.substr(0, bilateral_loc);
				std::string rhs_node_str = topology_item_str.substr(bilateral_loc + unidirectional_term.length());
				LOG(INFO) << "network topology: bilateral connect " << lhs_node_str << " with " << rhs_node_str;
				auto lhs_node_iter = node_container.find(lhs_node_str);
				auto rhs_node_iter = node_container.find(rhs_node_str);
				LOG_IF(FATAL, lhs_node_iter == node_container.end()) << lhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				LOG_IF(FATAL, rhs_node_iter == node_container.end()) << rhs_node_str << " is not found in nodes, raw topology: " << topology_item_str;
				
				if (check_duplicate_peer(*(lhs_node_iter->second), rhs_node_str))// connection already exists
				{
					LOG(WARNING) << rhs_node_str << " is already a peer of " << lhs_node_str;
				}
				else
				{
					auto &rhs_connected_node = node_container.at(rhs_node_str);
					lhs_node_iter->second->peers.push_back(rhs_connected_node);
				}
				
				if (check_duplicate_peer(*(rhs_node_iter->second), lhs_node_str))// connection already exists
				{
					LOG(WARNING) << lhs_node_str << " is already a peer of " << rhs_node_str;
				}
				else
				{
					auto &lhs_connected_node = node_container.at(lhs_node_str);
					rhs_node_iter->second->peers.push_back(lhs_connected_node);
				}
			}
			else
			{
				LOG(ERROR) << "unknown topology item: " << topology_item_str;
			}
		}
	}
	
	//load node reputation
	for (auto &target_node : node_container)
	{
		for (auto &reputation_node : node_container)
		{
			if (target_node.second->name != reputation_node.second->name)
			{
				target_node.second->reputation_map[reputation_node.second->name] = 1;
			}
		}
	}
	
	//load dataset
	Ml::data_converter<model_datatype> train_dataset;
	train_dataset.load_dataset_mnist(ml_train_dataset, ml_train_dataset_label);
	Ml::data_converter<model_datatype> test_dataset;
	test_dataset.load_dataset_mnist(ml_test_dataset, ml_test_dataset_label);
	
	std::ofstream drop_rate(output_path / "drop_rate.txt", std::ios::binary);
	std::mutex drop_rate_lock;
	
	//print reputation first line
	for (auto &single_node : node_container)
	{
		*single_node.second->reputation_output << "tick";
		for (auto &reputation_item: single_node.second->reputation_map)
		{
			*single_node.second->reputation_output << "," << reputation_item.first;
		}
		*single_node.second->reputation_output << std::endl;
	}
	

	
	//node vector container
	std::vector<node<model_datatype>*> node_vector_container;
	node_vector_container.reserve(node_container.size());
	for (auto &single_node : node_container)
	{
		node_vector_container.push_back(single_node.second);
	}
	
	//caffe solver for fedAvg process
	size_t solver_for_testing_size = std::thread::hardware_concurrency();
	auto* solver_for_testing = new Ml::MlCaffeModel<float, caffe::SGDSolver>[solver_for_testing_size];
	for (int i = 0; i < solver_for_testing_size; ++i)
	{
		solver_for_testing[i].load_caffe_model(ml_solver_proto);
	}
	
	//services
	std::unordered_map<std::string, std::shared_ptr<record_service<model_datatype>>> record_services;
	record_services.emplace("accuracy", new accuracy_record<model_datatype>());
	record_services.emplace("weights", new model_weights_record<model_datatype>());
	
	//accuracy service
	{
		auto service_iter = record_services.find("accuracy");
		std::static_pointer_cast<accuracy_record<model_datatype>>(service_iter->second)->ml_solver_proto = ml_solver_proto;
		std::static_pointer_cast<accuracy_record<model_datatype>>(service_iter->second)->ml_test_interval_tick = ml_test_interval_tick;
		std::static_pointer_cast<accuracy_record<model_datatype>>(service_iter->second)->test_dataset = &test_dataset;
		std::static_pointer_cast<accuracy_record<model_datatype>>(service_iter->second)->ml_test_batch_size = ml_test_batch_size;
		std::static_pointer_cast<accuracy_record<model_datatype>>(service_iter->second)->node_vector_container = &node_vector_container;
		service_iter->second->init_service(output_path, node_container);
	}
	
	//weights record
	{
		auto service_iter = record_services.find("weights");
		std::static_pointer_cast<model_weights_record<model_datatype>>(service_iter->second)->node_vector_container = &node_vector_container;
		std::static_pointer_cast<model_weights_record<model_datatype>>(service_iter->second)->ml_model_weight_diff_record_interval_tick = ml_model_weight_diff_record_interval_tick;
		service_iter->second->init_service(output_path, node_container);
	}
	
	////////////  BEGIN SIMULATION  ////////////
	{

		auto last_time_point = std::chrono::system_clock::now();
		

		
		int tick = 0;
		while (tick <= ml_max_tick)
		{
			std::cout << "tick: " << tick << " (" << ml_max_tick << ")" << std::endl;
			LOG(INFO) << "tick: " << tick << " (" << ml_max_tick << ")";
			
			if (tick != 0 && tick % report_time_remaining_per_tick_elapsed == 0)
			{
				auto now = std::chrono::system_clock::now();
				std::chrono::duration<float, std::milli> time_elapsed_ms = now - last_time_point;
				last_time_point = now;
				float speed_ms_per_tick = time_elapsed_ms.count() / float(report_time_remaining_per_tick_elapsed);
				std::chrono::milliseconds time_remain_ms(int(float(ml_max_tick - tick) * speed_ms_per_tick));
				std::time_t est_finish_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() + time_remain_ms);
				std::tm est_finish_time_tm = *std::localtime(&est_finish_time);
				std::cout << "est finish at: " << std::put_time( &est_finish_time_tm, "%Y-%m-%d %H:%M:%S") << std::endl;
			}
			
			bool exit = false;
			
			//train the model
			tmt::ParallelExecution_StepIncremental([&drop_rate_lock, &drop_rate, &tick, &train_dataset, &ml_train_batch_size, &ml_dataset_all_possible_labels](uint32_t index, uint32_t thread_index, node<model_datatype>* single_node){
				if (tick >= single_node->next_train_tick)
				{
					std::vector<Ml::tensor_blob_like<model_datatype>> train_data, train_label;
					std::tie(train_data, train_label) = get_dataset_by_node_type(train_dataset, *single_node, ml_train_batch_size, ml_dataset_all_possible_labels);
					
					std::random_device dev;
					std::mt19937 rng(dev());
					std::uniform_int_distribution<int> distribution(0, int(single_node->training_interval_tick.size()) - 1);
					single_node->next_train_tick += single_node->training_interval_tick[distribution(rng)];
					
					auto parameter_before = single_node->solver->get_parameter();
					single_node->train_model(train_data, train_label, true);
					auto output_opt = single_node->generate_model_sent();
					if (!output_opt)
					{
						LOG(INFO) << "ignore output for node " << single_node->name << " at tick " << tick;
						return;// Ignore the observer node since it does not train or send model to other nodes.
					}
					auto parameter_after = *output_opt;
					auto parameter_output = parameter_after;
					
					Ml::model_compress_type type;
					if (single_node->model_generation_type == Ml::model_compress_type::compressed_by_diff)
					{
						//drop models
						size_t total_weight = 0, dropped_count = 0;
						auto compressed_model = Ml::model_compress::compress_by_diff_get_model(parameter_before, parameter_after, single_node->filter_limit, &total_weight, &dropped_count);
						std::string compress_model_str = Ml::model_compress::compress_by_diff_lz_compress(compressed_model);
						{
							std::lock_guard guard(drop_rate_lock);
							drop_rate << "node:" << single_node->name << "    tick:" << tick << "    drop:" << ((float) dropped_count) / float(total_weight) << "(" << dropped_count << "/" << total_weight << ")"
							          << "    compressed_size:" << compress_model_str.size() << std::endl;
						}
						parameter_output = compressed_model;
						type = Ml::model_compress_type::compressed_by_diff;
					}
					else
					{
						type = Ml::model_compress_type::normal;
					}
					
					//add ML network to FedAvg buffer
					for (auto updating_node : single_node->peers)
					{
						std::lock_guard guard(updating_node->parameter_buffer_lock);
						updating_node->parameter_buffer.emplace_back(single_node->name, type, parameter_output);
					}
					
				}
			}, node_vector_container.size(), node_vector_container.data());
			
			//check fedavg buffer full
			tmt::ParallelExecution_StepIncremental([&tick,&test_dataset,&ml_test_batch_size,&ml_dataset_all_possible_labels,&solver_for_testing](uint32_t index, uint32_t thread_index, node<model_datatype>* single_node){
				if (single_node->parameter_buffer.size() >= single_node->buffer_size)
				{
					//update model
					auto parameter = single_node->solver->get_parameter();
					std::vector<updated_model<model_datatype>> received_models;
					received_models.resize(single_node->parameter_buffer.size());
					for (int i = 0; i < received_models.size(); ++i)
					{
						auto[node_name, type, model] = single_node->parameter_buffer[i];
						received_models[i].model_parameter = model;
						received_models[i].type = type;
						received_models[i].generator_address = node_name;
						received_models[i].accuracy = 0;
					}
					
					float self_accuracy = 0;
					std::thread self_accuracy_thread([&single_node, &test_dataset, &ml_test_batch_size, &self_accuracy, &ml_dataset_all_possible_labels]()
					                                 {
						                                 //auto [test_data, test_label] = test_dataset.get_random_data(ml_test_batch_size);
						                                 auto[test_data, test_label] = get_dataset_by_node_type(test_dataset, *single_node, ml_test_batch_size, ml_dataset_all_possible_labels);
						                                 self_accuracy = single_node->solver->evaluation(test_data, test_label);
					                                 });
					size_t worker = received_models.size() > std::thread::hardware_concurrency() ? std::thread::hardware_concurrency() : received_models.size();
					auto_multi_thread::ParallelExecution_with_thread_index(worker, [parameter, &single_node, &solver_for_testing, &test_dataset, &ml_test_batch_size, &ml_dataset_all_possible_labels](uint32_t index, uint32_t thread_index, updated_model<model_datatype> &model)
					{
						auto output_model = parameter;
						if (model.type == Ml::model_compress_type::compressed_by_diff)
						{
							output_model.patch_weight(model.model_parameter);
						}
						else if (model.type == Ml::model_compress_type::normal)
						{
							output_model = model.model_parameter;
						}
						else
						{
							LOG(FATAL) << "unknown model type";
						}
						solver_for_testing[thread_index].set_parameter(output_model);
						auto[test_data, test_label] = get_dataset_by_node_type(test_dataset, *single_node, ml_test_batch_size, ml_dataset_all_possible_labels);
						model.accuracy = solver_for_testing[thread_index].evaluation(test_data, test_label);
					}, received_models.size(), received_models.data());
					self_accuracy_thread.join();
					std::string log_msg = (boost::format("tick: %1%, node: %2%, accuracy: %3%") % tick % single_node->name % self_accuracy).str();
					std::cout << log_msg << std::endl;
					LOG(INFO) << log_msg;
					auto &reputation_map = single_node->reputation_map;
					reputation_dll.get()->update_model(parameter, self_accuracy, received_models, reputation_map);
					single_node->solver->set_parameter(parameter);
					
					//print reputation map
					*single_node->reputation_output << tick;
					for (const auto &reputation_pair : reputation_map)
					{
						*single_node->reputation_output << "," << reputation_pair.second;
					}
					*single_node->reputation_output << std::endl;
					
					//clear buffer and start new loop
					single_node->parameter_buffer.clear();
				}
			}, node_vector_container.size(), node_vector_container.data());
			
			//services
			for (auto& [name, service_instance]: record_services)
			{
				service_instance->process_per_tick(tick, node_container);
			}
			
			tick++;
			if (exit) break;
		}
	}
	
	for (auto& [name, service_instance]: record_services)
	{
		service_instance->destruction_service();
	}
	
	delete[] solver_for_testing;
	
	drop_rate.flush();
	drop_rate.close();
	
	return 0;
}
