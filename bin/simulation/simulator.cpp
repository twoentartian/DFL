#include <thread>
#include <unordered_map>
#include <fstream>

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
	
	std::string name;
	dataset_mode_type dataset_mode;
	std::unordered_map<int, std::tuple<Ml::caffe_parameter_net<model_datatype>, float>> nets_record;
	int next_train_tick;
	std::vector<int> non_iid_high_labels;
	std::vector<int> training_interval_tick;
	
	size_t buffer_size;
	std::vector<std::tuple<std::string, model_type, Ml::caffe_parameter_net<model_datatype>>> parameter_buffer;
	std::shared_ptr<Ml::MlCaffeModel<model_datatype, caffe::SGDSolver>> solver;
	std::unordered_map<std::string, double> reputation_map;
	model_type model_generation_type;
	float filter_limit;
};

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
	config.LoadConfiguration("./simulator_config.json");
	auto config_json = config.get_json();
	
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
	auto ml_non_iid_higher_frequency_lower = *config.get<model_datatype>("ml_non_iid_higher_frequency_lower");
	auto ml_non_iid_higher_frequency_upper = *config.get<model_datatype>("ml_non_iid_higher_frequency_upper");
	auto ml_non_iid_lower_frequency_lower = *config.get<model_datatype>("ml_non_iid_lower_frequency_lower");
	auto ml_non_iid_lower_frequency_upper = *config.get<model_datatype>("ml_non_iid_lower_frequency_upper");
	std::vector<int> ml_dataset_all_possible_labels;
	for (auto& el : config_json["ml_dataset_all_possible_labels"])
	{
		ml_dataset_all_possible_labels.push_back(el);
	}
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
		
		//load models solver
		iter->second.solver->load_caffe_model(ml_solver_proto);
		
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
		if (model_generation_type_str == "filtered")
		{
			iter->second.model_generation_type = model_type::filtered;
		}
		else if (model_generation_type_str == "normal")
		{
			iter->second.model_generation_type = model_type::normal;
		}
		else
		{
			LOG(FATAL) << "unknown model_generation_type:" << model_generation_type_str;
			return -1;
		}
		
		//filter_limit
		iter->second.filter_limit = single_node["filter_limit"];
		
		//non_iid_high_labels
		for (auto& el : single_node["non_iid_high_labels"])
		{
			iter->second.non_iid_high_labels.push_back(el);
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
				target_node.second.reputation_map[reputation_node.second.name] = 100;
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
	////////////  BEGIN SIMULATION  ////////////
	int tick = 0;
	while (tick <= ml_max_tick)
	{
		std::cout << "tick: " << tick << " (" << ml_max_tick << ")" << std::endl;
		bool exit = false;
		for (auto& single_node : node_container)
		{
			Ml::tensor_blob_like<model_datatype> label;
			label.getShape() = {1};
			
			std::vector<Ml::tensor_blob_like<model_datatype>> train_data,train_label;
			if (single_node.second.dataset_mode == dataset_mode_type::default_dataset)
			{
				//iid dataset
				std::tie(train_data, train_label) = train_dataset.get_random_data(ml_train_batch_size);
			}
			else if (single_node.second.dataset_mode == dataset_mode_type::iid_dataset)
			{
				std::random_device dev;
				std::mt19937 rng(dev());
				std::uniform_int_distribution<int> distribution(0, int (ml_dataset_all_possible_labels.size()) - 1);
				for (int i = 0; i < ml_train_batch_size; ++i)
				{
					int label_int = ml_dataset_all_possible_labels[distribution(rng)];
					label.getData() = {model_datatype (label_int)};
					auto [train_data_slice, train_label_slice] = train_dataset.get_random_data_by_Label(label, 1);
					train_data.insert(train_data.end(), train_data_slice.begin(), train_data_slice.end());
					train_label.insert(train_label.end(), train_label_slice.begin(), train_label_slice.end());
				}
			}
			else if (single_node.second.dataset_mode == dataset_mode_type::non_iid_dataset)
			{
				//non-iid dataset
				std::random_device dev;
				std::mt19937 rng(dev());
				std::uniform_real_distribution<model_datatype> distribution(ml_non_iid_lower_frequency_lower,ml_non_iid_lower_frequency_upper);
				std::uniform_real_distribution<model_datatype> dense_distribution(ml_non_iid_higher_frequency_lower,ml_non_iid_higher_frequency_upper);
				Ml::non_iid_distribution<model_datatype> non_iid_distribution;
				for (auto& target_label : ml_dataset_all_possible_labels)
				{
					label.getData() = {model_datatype (target_label)};
					auto iter = std::find(single_node.second.non_iid_high_labels.begin(), single_node.second.non_iid_high_labels.end(), target_label);
					if(iter != single_node.second.non_iid_high_labels.end())
					{
						//higher frequency
						non_iid_distribution.add_distribution(label, dense_distribution(rng));
					}
					else
					{
						//lower frequency
						non_iid_distribution.add_distribution(label, distribution(rng));
					}
				}
				std::tie(train_data, train_label) = train_dataset.get_random_non_iid_dataset(non_iid_distribution, ml_train_batch_size);
			}
			
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
				model_type type;
				if (single_node.second.model_generation_type == model_type::filtered)
				{
					auto parameter_diff = parameter_after - parameter_before;
					
					//drop models
					auto& layers = parameter_diff.getLayers();
					for (int i = 0; i < layers.size(); ++i)
					{
						auto& blob = layers[i].getBlob_p();
						auto& blob_data = blob->getData();
						if (blob_data.empty())
						{
							continue;
						}
						auto [max, min] = find_max_min(blob_data.data(), blob_data.size());
						auto range = max - min;
						auto lower_bound = min + range * single_node.second.filter_limit * 0.5;
						auto higher_bound = max - range * single_node.second.filter_limit * 0.5;
						
						int drop_count = 0;
						for (auto&& data: blob_data)
						{
							if(data < higher_bound && data > lower_bound)
							{
								//drop
								drop_count++;
								data = NAN;
							}
						}
						drop_rate << "node:" << single_node.second.name << "    iter:" << tick << "    layer:" << i << "    drop:" << ((float)drop_count) / blob_data.size() << "(" << drop_count << "/" << blob_data.size() << ")" << std::endl;
					}
					parameter_output = parameter_diff.dot_divide(parameter_diff).dot_product(parameter_after);
					type = model_type::filtered;
				}
				else
				{
					type = model_type::normal;
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
							std::thread self_accuracy_thread([&updating_node, &test_dataset, &ml_test_batch_size, &self_accuracy](){
								auto [test_data, test_label] = test_dataset.get_random_data(ml_test_batch_size);
								self_accuracy = updating_node.second.solver->evaluation(test_data, test_label);
							});
							size_t worker = received_models.size() > std::thread::hardware_concurrency()?std::thread::hardware_concurrency():received_models.size();
							auto_multi_thread::ParallelExecution(worker, [parameter, &solver_for_testing, &test_dataset, &ml_test_batch_size](uint32_t index, updated_model<model_datatype>& model){
								auto output_model = parameter;
								if (model.type == model_type::filtered)
								{
									output_model.patch_weight(model.model_parameter);
								}
								else if (model.type == model_type::normal)
								{
									output_model = model.model_parameter;
								}
								else
								{
									LOG(FATAL) << "unknown model type";
								}
								solver_for_testing[index].set_parameter(output_model);
								auto [test_data, test_label] = test_dataset.get_random_data(ml_test_batch_size);
								model.accuracy = solver_for_testing[index].evaluation(test_data, test_label);
							}, received_models.size(), received_models.data());
							self_accuracy_thread.join();
							std::cout << (boost::format("tick: %1%, node: %2%, accuracy: %3%") % tick % single_node.second.name % self_accuracy).str() << std::endl;
							auto& reputation_map = updating_node.second.reputation_map;
							reputation_dll.get()->update_model(parameter, self_accuracy, received_models, reputation_map);
							updating_node.second.solver->set_parameter(parameter);
							
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
		std::cout << "testing - 0%";
		auto_multi_thread::ParallelExecution_with_thread_index(worker, [&current_progress, &total, &progress_counter, &solver_for_final_testing, &test_dataset, &ml_test_batch_size](uint32_t index, uint32_t thread_index, std::tuple<Ml::caffe_parameter_net<model_datatype>, float>& modeL_accuracy){
			auto& [model, accuracy] = modeL_accuracy;
			auto [test_data, test_label] = test_dataset.get_random_data(ml_test_batch_size);
			solver_for_final_testing[thread_index].set_parameter(model);
			accuracy = solver_for_final_testing[thread_index].evaluation(test_data, test_label);
			
			progress_counter++;
			if (int (float(progress_counter) / float (total) * 100.0) > current_progress)
			{
				std::cout << "\r                    ";
				std::cout << "testing - " << current_progress << "%";
				current_progress += step;
			}
		}, test_modeL_container.size(), test_modeL_container.data());
		std::cout << "\r                    ";
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
	
	//print accuracy
	for (auto& single_node : node_container)
	{
		std::cout << "node name: "<< single_node.second.name << std::endl;
		for (auto& single_model: single_node.second.nets_record)
		{
			auto& [model, accuracy] = single_model.second;
			std::cout << "tick: "<< single_model.first << "  accuracy: " << accuracy << std::endl;
		}
	}
	

	return 0;
}