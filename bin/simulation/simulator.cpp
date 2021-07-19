#include <thread>
#include <unordered_map>
#include <fstream>
#include <set>

#include <glog/logging.h>

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

/** assumptions in this simulator:
 *  (1) no malicious transactions, no malicious node
 *  (2) no transaction transmission time
 *
 *
 */

using model_datatype = float;

enum class dataset_mode_type
{
	unknown,
	default_dataset,
	iid_dataset,
	non_iid_dataset
};

class node
{
public:
	node(std::string  _name, size_t buf_size) : name(std::move(_name)), next_train_tick(0), buffer_size(buf_size), dataset_mode(dataset_mode_type::unknown)
	{
		solver.reset(new Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>());
	}
	
	~node()
	{
		if (reputation_output)
		{
			reputation_output->flush();
			reputation_output->close();
		}
	}
	
	void open_reputation_file(const std::filesystem::path& output_path)
	{
		reputation_output.reset(new std::ofstream());
		reputation_output->open(output_path / (name + "_reputation.txt"));
	}
	
	std::string name;
	dataset_mode_type dataset_mode;
	std::unordered_map<int, std::tuple<Ml::caffe_parameter_net<model_datatype>, float>> nets_record;
	int next_train_tick;
	std::unordered_map<int, std::tuple<float, float>> special_non_iid_distribution; //label:{min,max}
	std::vector<int> training_interval_tick;
	
	size_t buffer_size;
	std::vector<std::tuple<std::string, Ml::model_compress_type, Ml::caffe_parameter_net<model_datatype>>> parameter_buffer;
	std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> solver;
	std::unordered_map<std::string, double> reputation_map;
	Ml::model_compress_type model_generation_type;
	float filter_limit;
	std::shared_ptr<std::ofstream> reputation_output;
};

//return: train_data,train_label
std::tuple<std::vector<Ml::tensor_blob_like<model_datatype>>,std::vector<Ml::tensor_blob_like<model_datatype>>> get_dataset_by_node_type(Ml::data_converter<model_datatype>& dataset, const node& target_node, int size, const std::vector<int>& ml_dataset_all_possible_labels)
{
	Ml::tensor_blob_like<model_datatype> label;
	label.getShape() = {1};
	std::vector<Ml::tensor_blob_like<model_datatype>> train_data,train_label;
	if (target_node.dataset_mode == dataset_mode_type::default_dataset)
	{
		//iid dataset
		std::tie(train_data, train_label) = dataset.get_random_data(size);
	}
	else if (target_node.dataset_mode == dataset_mode_type::iid_dataset)
	{
		std::random_device dev;
		std::mt19937 rng(dev());
		std::uniform_int_distribution<int> distribution(0, int (ml_dataset_all_possible_labels.size()) - 1);
		for (int i = 0; i < size; ++i)
		{
			int label_int = ml_dataset_all_possible_labels[distribution(rng)];
			label.getData() = {model_datatype (label_int)};
			auto [train_data_slice, train_label_slice] = dataset.get_random_data_by_Label(label, 1);
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
		for (auto& target_label : ml_dataset_all_possible_labels)
		{
			label.getData() = {model_datatype (target_label)};
			auto iter = target_node.special_non_iid_distribution.find(target_label);
			if (iter != target_node.special_non_iid_distribution.end())
			{
				auto [dis_min,dis_max] = iter->second;
				std::uniform_real_distribution<model_datatype> distribution(dis_min, dis_max);
				label_distribution.add_distribution(label, distribution(rng));
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

std::unordered_map<std::string, node> node_container;
dll_loader<reputation_interface<model_datatype>> reputation_dll;

int main(int argc, char *argv[])
{
	//create new folder
	std::string time_str = time_util::time_to_text(time_util::get_current_utc_time());
	std::filesystem::path output_path = std::filesystem::current_path() / time_str;
	std::filesystem::create_directories(output_path);
	
	//log file path
	google::InitGoogleLogging(argv[0]);
	std::filesystem::path log_path(output_path / "log");
	if (!std::filesystem::exists(log_path)) std::filesystem::create_directories(log_path);
	google::SetLogDestination(google::INFO, (log_path.string() + "/").c_str());
	
	//load configuration
	configuration_file config;
	config.SetDefaultConfiguration(get_default_simulation_configuration());
	auto load_config_rc = config.LoadConfiguration("./simulator_config.json");
	if (load_config_rc < 0)
	{
		LOG(FATAL) << "cannot load configuration file, wrong format?";
		return -1;
	}
	auto config_json = config.get_json();
	//backup configuration file
	std::filesystem::copy("./simulator_config.json", output_path / "simulator_config.json");
	
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
	
	std::vector<int> ml_dataset_all_possible_labels = *config.get_vec<int>("ml_dataset_all_possible_labels");
	std::vector<float> ml_non_iid_normal_weight = *config.get_vec<float>("ml_non_iid_normal_weight");
	LOG_IF(ERROR, ml_non_iid_normal_weight.size() != 2) << "ml_non_iid_normal_weight must be a two-value array, {max min}";
	auto ml_reputation_dll_path = *config.get<std::string>("ml_reputation_dll_path");
	
	//load reputation dll
	if constexpr(std::is_same_v<model_datatype, float>)
	{
		auto [status, msg] = reputation_dll.load(ml_reputation_dll_path, export_class_name_reputation_float);
		LOG_IF(FATAL, !status) << "error to load reputation dll: " << msg;
	}
	else if constexpr(std::is_same_v<model_datatype, double>)
	{
		auto [status, msg] = reputation_dll.load(ml_reputation_dll_path, export_class_name_reputation_double);
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
	auto number_of_nodes = nodes_json.size();
	size_t max_buffer_size = 0;
	for (auto& single_node: nodes_json)
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
		auto [iter, status] = node_container.emplace(node_name, node(node_name, buf_size));
		
		//load models solver and open reputation_file
		iter->second.solver->load_caffe_model(ml_solver_proto);
		iter->second.open_reputation_file(output_path);
		
		//dataset mode
		const std::string dataset_mode_str = single_node["dataset_mode"];
		if ( dataset_mode_str == "default" )
		{
			iter->second.dataset_mode = dataset_mode_type::default_dataset;
		}
		else if ( dataset_mode_str == "iid")
		{
			iter->second.dataset_mode = dataset_mode_type::iid_dataset;
		}
		else if ( dataset_mode_str == "non-iid")
		{
			iter->second.dataset_mode = dataset_mode_type::non_iid_dataset;
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
			iter->second.model_generation_type = Ml::model_compress_type::compressed_by_diff;
		}
		else if (model_generation_type_str == "normal")
		{
			iter->second.model_generation_type = Ml::model_compress_type::normal;
		}
		else
		{
			LOG(FATAL) << "unknown model_generation_type:" << model_generation_type_str;
			return -1;
		}
		
		//filter_limit
		iter->second.filter_limit = single_node["filter_limit"];
		
		//label_distribution
		std::string dataset_mode = single_node["dataset_mode"];
		if (dataset_mode == "iid")
		{
			//nothing to do because the single_node["non_iid_distribution"] will not be used
		}
		else if(dataset_mode == "non-iid")
		{
			configuration_file::json non_iid_distribution = single_node["non_iid_distribution"];
			for (auto non_iid_item = non_iid_distribution.begin(); non_iid_item != non_iid_distribution.end(); ++non_iid_item)
			{
				int label = std::stoi(non_iid_item.key());
				auto min_max_array = *non_iid_item;
				float min = min_max_array.at(0);
				float max = min_max_array.at(1);
				iter->second.special_non_iid_distribution[label] = {min,max};
			}
			for (auto& el: ml_dataset_all_possible_labels)
			{
				auto iter_el = iter->second.special_non_iid_distribution.find(el);
				if (iter_el == iter->second.special_non_iid_distribution.end())
				{
					//not set before
					iter->second.special_non_iid_distribution[el] = {ml_non_iid_normal_weight[0],ml_non_iid_normal_weight[1]};
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
		for (auto& el : single_node["training_interval_tick"])
		{
			iter->second.training_interval_tick.push_back(el);
		}
	}
	
	//load node reputation
	for (auto& target_node : node_container)
	{
		for (auto& reputation_node : node_container)
		{
			if (target_node.second.name != reputation_node.second.name)
			{
				target_node.second.reputation_map[reputation_node.second.name] = 1;
			}
		}
	}
	
	//solver for testing
	auto* solver_for_testing = new Ml::MlCaffeModel<float, caffe::SGDSolver>[max_buffer_size];
	for (int i = 0; i < max_buffer_size; ++i)
	{
		solver_for_testing[i].load_caffe_model(ml_solver_proto);
	}
	
	//load dataset
	Ml::data_converter<model_datatype> train_dataset;
	train_dataset.load_dataset_mnist(ml_train_dataset,ml_train_dataset_label);
	Ml::data_converter<model_datatype> test_dataset;
	test_dataset.load_dataset_mnist(ml_test_dataset,ml_test_dataset_label);
	
	std::ofstream drop_rate(output_path / "drop_rate.txt", std::ios::binary);
	
	//print reputation first line
	for (auto& single_node : node_container)
	{
		*single_node.second.reputation_output << "tick";
		for (auto& reputation_item: single_node.second.reputation_map)
		{
			*single_node.second.reputation_output << "," << reputation_item.first;
		}
		*single_node.second.reputation_output << std::endl;
	}
	
	////////////  BEGIN SIMULATION  ////////////
	int tick = 0;
	while (tick <= ml_max_tick)
	{
		std::cout << "tick: " << tick << " (" << ml_max_tick << ")" << std::endl;
		bool exit = false;
		for (auto& single_node : node_container)
		{
			std::vector<Ml::tensor_blob_like<model_datatype>> train_data,train_label;
			std::tie(train_data, train_label) = get_dataset_by_node_type(train_dataset, single_node.second, ml_train_batch_size, ml_dataset_all_possible_labels);
			
			//train
			if (tick >= single_node.second.next_train_tick)
			{
				std::random_device dev;
				std::mt19937 rng(dev());
				std::uniform_int_distribution<int> distribution(0, int (single_node.second.training_interval_tick.size()) - 1);
				single_node.second.next_train_tick += single_node.second.training_interval_tick[distribution(rng)];
				
				auto parameter_before = single_node.second.solver->get_parameter();
				single_node.second.solver->train(train_data,train_label);
				auto parameter_after = single_node.second.solver->get_parameter();
				auto parameter_output = parameter_after;
				Ml::model_compress_type type;
				if (single_node.second.model_generation_type == Ml::model_compress_type::compressed_by_diff)
				{
					//drop models
					size_t total_weight = 0, dropped_count = 0;
					auto compressed_model = Ml::model_compress::compress_by_diff_get_model(parameter_before, parameter_after, single_node.second.filter_limit, &total_weight, &dropped_count);
					std::string compress_model_str = Ml::model_compress::compress_by_diff_lz_compress(compressed_model);
					drop_rate << "node:" << single_node.second.name << "    tick:" << tick << "    drop:" << ((float)dropped_count) / total_weight << "(" << dropped_count << "/" << total_weight << ")"
					 << "    compressed_size:" << compress_model_str.size() << std::endl;
					
					parameter_output = compressed_model;
					type = Ml::model_compress_type::compressed_by_diff;
				}
				else
				{
					type = Ml::model_compress_type::normal;
				}
				
				//add to buffer, and update model if necessary
				for (auto& updating_node : node_container)
				{
					if (single_node.second.name != updating_node.second.name)
					{
						updating_node.second.parameter_buffer.emplace_back(single_node.second.name, type, parameter_output);
						if (updating_node.second.parameter_buffer.size() == updating_node.second.buffer_size)
						{
							//update model
							auto parameter = updating_node.second.solver->get_parameter();
							std::vector<updated_model<model_datatype>> received_models;
							received_models.resize(updating_node.second.parameter_buffer.size());
							for (int i = 0; i < received_models.size(); ++i)
							{
								auto [node_name, type, model] = updating_node.second.parameter_buffer[i];
								received_models[i].model_parameter = model;
								received_models[i].type = type;
								received_models[i].generator_address = node_name;
								received_models[i].accuracy = 0;
							}
							
							float self_accuracy = 0;
							std::thread self_accuracy_thread([&updating_node, &test_dataset, &ml_test_batch_size, &self_accuracy, &ml_dataset_all_possible_labels](){
								//auto [test_data, test_label] = test_dataset.get_random_data(ml_test_batch_size);
								auto [test_data, test_label] = get_dataset_by_node_type(test_dataset, updating_node.second, ml_test_batch_size, ml_dataset_all_possible_labels);
								self_accuracy = updating_node.second.solver->evaluation(test_data, test_label);
							});
							size_t worker = received_models.size() > std::thread::hardware_concurrency()?std::thread::hardware_concurrency():received_models.size();
							auto_multi_thread::ParallelExecution(worker, [parameter, &updating_node, &solver_for_testing, &test_dataset, &ml_test_batch_size, &ml_dataset_all_possible_labels](uint32_t index, updated_model<model_datatype>& model){
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
								solver_for_testing[index].set_parameter(output_model);
								auto [test_data, test_label] = get_dataset_by_node_type(test_dataset, updating_node.second, ml_test_batch_size, ml_dataset_all_possible_labels);
								model.accuracy = solver_for_testing[index].evaluation(test_data, test_label);
							}, received_models.size(), received_models.data());
							self_accuracy_thread.join();
							std::cout << (boost::format("tick: %1%, node: %2%, accuracy: %3%") % tick % updating_node.second.name % self_accuracy).str() << std::endl;
							auto& reputation_map = updating_node.second.reputation_map;
							reputation_dll.get()->update_model(parameter, self_accuracy, received_models, reputation_map);
							updating_node.second.solver->set_parameter(parameter);
							
							//print reputation map
							*updating_node.second.reputation_output << tick;
							for (const auto& reputation_pair : reputation_map)
							{
								*updating_node.second.reputation_output << "," << reputation_pair.second;
							}
							*updating_node.second.reputation_output << std::endl;
							
							//clear buffer and start new loop
							updating_node.second.parameter_buffer.clear();
						}
					}
				}
			}
			
			//mark testing
			if (tick % ml_test_interval_tick == 0)
			{
				single_node.second.nets_record.emplace(tick, std::make_tuple(single_node.second.solver->get_parameter(), 0.0));
			}
		}
		
		tick ++;
		if (exit) break;
	}
	delete[] solver_for_testing;
	
	drop_rate.flush();
	drop_rate.close();
	
	//calculating accuracy
	std::cout << "Begin calculating accuracy" << std::endl;
	{
		size_t worker = std::thread::hardware_concurrency();
		auto* solver_for_final_testing = new Ml::MlCaffeModel<model_datatype , caffe::SGDSolver>[worker];
		for (int i = 0; i < worker; ++i)
		{
			solver_for_final_testing[i].load_caffe_model(ml_solver_proto);
		}
		
		std::vector<std::tuple<Ml::caffe_parameter_net<model_datatype>, model_datatype>> test_modeL_container;
		for (auto& single_node : node_container)
		{
			for (auto& single_model: single_node.second.nets_record)
			{
				test_modeL_container.push_back(single_model.second);
			}
		}
		
		//testing
		const size_t total = test_modeL_container.size();
		std::atomic_int progress_counter = 0;
		std::atomic_int current_progress = 0;
		constexpr int step = 5;
		auto_multi_thread::ParallelExecution_with_thread_index(worker, [&current_progress, &total, &progress_counter, &solver_for_final_testing, &test_dataset, &ml_test_batch_size](uint32_t index, uint32_t thread_index, std::tuple<Ml::caffe_parameter_net<model_datatype>, float>& modeL_accuracy){
			auto& [model, accuracy] = modeL_accuracy;
			auto [test_data, test_label] = test_dataset.get_random_data(ml_test_batch_size);
			solver_for_final_testing[thread_index].set_parameter(model);
			accuracy = solver_for_final_testing[thread_index].evaluation(test_data, test_label);
			
			progress_counter++;
			if (int (float(progress_counter) / float (total) * 100.0) > current_progress)
			{
				std::cout << "testing - " << current_progress << "%" << std::endl;
				std::cout.flush();
				current_progress += step;
			}
		}, test_modeL_container.size(), test_modeL_container.data());
		std::cout << "testing - 100%" << std::endl;
		
		//apply back to map
		size_t counter = 0;
		for (auto& single_node : node_container)
		{
			for (auto& single_model: single_node.second.nets_record)
			{
				single_model.second = test_modeL_container[counter];
				counter++;
			}
		}
		
		delete[] solver_for_final_testing;
	}
	
	//print accuracy to file
	{
		std::ofstream accuracy_file(output_path/"accuracy.csv", std::ios::binary);
		std::vector<int> all_ticks;
		for (auto& single_node : node_container)
		{
			for (auto& single_model: single_node.second.nets_record)
			{
				all_ticks.push_back(single_model.first);
			}
		}
		std::set<int> all_ticks_set;
		for(int & all_tick : all_ticks) all_ticks_set.insert( all_tick );
		all_ticks.assign(all_ticks_set.begin(), all_ticks_set.end());
		std::sort(all_ticks.begin(), all_ticks.end());
		
		//print first line
		accuracy_file << "tick";
		for (auto& single_node : node_container)
		{
			accuracy_file << "," << single_node.second.name;
		}
		accuracy_file << std::endl;
		
		//print accuracy
		for (auto current_tick: all_ticks)
		{
			accuracy_file << current_tick;
			for (auto& single_node : node_container)
			{
				auto iter_find = single_node.second.nets_record.find(current_tick);
				if (iter_find != single_node.second.nets_record.end())
				{
					auto& [model, accuracy] = iter_find->second;
					accuracy_file << "," << accuracy;
				}
				else
				{
					accuracy_file << "," << " ";
				}
			}
			accuracy_file << std::endl;
		}
		
		accuracy_file.flush();
		accuracy_file.close();
	}

	return 0;
}