#pragma once

#include <string>
#include <random>
#include <configure_file.hpp>
#include <sstream>
#include "./node.hpp"
#include "./simulation_util.hpp"

enum class record_service_status
{
	success,
	fail_not_specified_reason,
	skipped
	
};

template <typename model_datatype>
class service
{
public:
	bool enable;
	
	virtual std::tuple<record_service_status, std::string> apply_config(const configuration_file::json& config) = 0;
	
	service()
	{
		node_vector_container = nullptr;
		node_container = nullptr;
		enable = false;
	}
	
	virtual std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *>&, std::vector<node<model_datatype>*>&) = 0;
	
	virtual std::tuple<record_service_status, std::string> process_per_tick(int tick) = 0;
	
	virtual std::tuple<record_service_status, std::string> destruction_service() = 0;

protected:
	void set_node_container(std::unordered_map<std::string, node<model_datatype> *>& map, std::vector<node<model_datatype>*>& vector)
	{
		node_vector_container = &vector;
		node_container = &map;
	}
	
	std::vector<node<model_datatype>*>* node_vector_container;
	std::unordered_map<std::string, node<model_datatype> *>* node_container;
};

template <typename model_datatype>
class accuracy_record : public service<model_datatype>
{
public:
	//set these variables before init
	std::string ml_solver_proto;
	int ml_test_interval_tick;
	Ml::data_converter<model_datatype>* test_dataset;
	int ml_test_batch_size;
	
	accuracy_record()
	{
		ml_test_batch_size = 0;
		ml_test_interval_tick = 0;
		ml_solver_proto = "";
		test_dataset = nullptr;
		solver_for_testing = nullptr;
	}
	
	std::tuple<record_service_status, std::string> apply_config(const configuration_file::json& config) override
	{
		this->enable = config["enable"];
		this->ml_test_interval_tick = config["interval"];
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *>& _node_container, std::vector<node<model_datatype>*>& _node_vector_container) override
	{
		this->set_node_container(_node_container, _node_vector_container);
		
		LOG_IF(FATAL, test_dataset == nullptr) << "test_dataset is not set";
		
		accuracy_file.reset(new std::ofstream(output_path / "accuracy.csv", std::ios::binary));
		
		*accuracy_file << "tick";
		for (auto &single_node : *(this->node_container))
		{
			*accuracy_file << "," << single_node.second->name;
		}
		*accuracy_file << std::endl;
		
		//solver for testing
		size_t solver_for_testing_size = std::thread::hardware_concurrency();
		solver_for_testing = new Ml::MlCaffeModel<float, caffe::SGDSolver>[solver_for_testing_size];
		for (int i = 0; i < solver_for_testing_size; ++i)
		{
			solver_for_testing[i].load_caffe_model(ml_solver_proto);
		}
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> process_per_tick(int tick) override
	{
		if (this->enable == false) return {record_service_status::skipped, "not enabled"};
		
		if (tick % ml_test_interval_tick == 0)
		{
			tmt::ParallelExecution([&tick, this](uint32_t index, uint32_t thread_index, node<model_datatype> *single_node)
			                       {
				                       auto[test_data, test_label] = test_dataset->get_random_data(ml_test_batch_size);
				                       auto model = single_node->solver->get_parameter();
				                       solver_for_testing[thread_index].set_parameter(model);
				                       auto accuracy = solver_for_testing[thread_index].evaluation(test_data, test_label);
				                       single_node->nets_accuracy_only_record.emplace(tick, accuracy);
			                       }, this->node_vector_container->size(), this->node_vector_container->data());

			//print accuracy to file
			*accuracy_file << tick;
			for (auto &single_node : *(this->node_container))
			{
				auto iter_find = single_node.second->nets_accuracy_only_record.find(tick);
				if (iter_find != single_node.second->nets_accuracy_only_record.end())
				{
					auto accuracy = iter_find->second;
					*accuracy_file << "," << accuracy;
				}
				else
				{
					*accuracy_file << "," << " ";
				}
			}
			*accuracy_file << std::endl;
		}
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> destruction_service() override
	{
		delete[] solver_for_testing;
		accuracy_file->flush();
		accuracy_file->close();
		
		return {record_service_status::success, ""};
	}
	
private:
	Ml::MlCaffeModel<float, caffe::SGDSolver>* solver_for_testing;
	std::shared_ptr<std::ofstream> accuracy_file;
};

template <typename model_datatype>
class model_weights_record : public service<model_datatype>
{
public:
	//set these variables before init
	int ml_model_weight_diff_record_interval_tick;
	
	model_weights_record()
	{
		this->node_vector_container = nullptr;
		ml_model_weight_diff_record_interval_tick = 0;
	}
	
	std::tuple<record_service_status, std::string> apply_config(const configuration_file::json& config) override
	{
		this->enable = config["enable"];
		this->ml_model_weight_diff_record_interval_tick = config["interval"];
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *>& _node_container, std::vector<node<model_datatype>*>& _node_vector_container) override
	{
		//LOG_IF(FATAL, node_vector_container == nullptr) << "node_vector_container is not set";
		this->set_node_container(_node_container, _node_vector_container);
		
		model_weights_file.reset(new std::ofstream(output_path / "model_weight_diff.csv", std::ios::binary));
		*model_weights_file << "tick";
		
		auto weights = (*this->node_vector_container)[0]->solver->get_parameter();
		auto layers = weights.getLayers();
		
		for (auto& single_layer: layers)
		{
			*model_weights_file << "," << single_layer.getName();
		}
		*model_weights_file << std::endl;
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> process_per_tick(int tick) override
	{
		if (this->enable == false) return {record_service_status::skipped, "not enabled"};
		
		if (tick % ml_model_weight_diff_record_interval_tick == 0)
		{
			auto weights = (*this->node_vector_container)[0]->solver->get_parameter();
			auto layers = weights.getLayers();
			size_t number_of_layers = layers.size();
			auto* weight_diff_sums = new std::atomic<float>[number_of_layers];
			for (int i = 0; i < number_of_layers; ++i) weight_diff_sums[i] = 0;
			
			tmt::ParallelExecution([&tick, this, &weight_diff_sums,&number_of_layers](uint32_t index, uint32_t thread_index, node<model_datatype> *single_node)
			                       {
				                       uint32_t index_next = index + 1;
				                       const uint32_t total_size = this->node_vector_container->size();
				                       if (index_next == total_size-1) index_next = 0;
				                       auto net1 = (*this->node_vector_container)[index]->solver->get_parameter();
				                       auto net2 = (*this->node_vector_container)[index_next]->solver->get_parameter();
				                       auto layers1 = net1.getLayers();
				                       auto layers2 = net2.getLayers();
				                       for (int i = 0; i < number_of_layers; ++i)
				                       {
					                       auto diff = layers1[i] - layers2[i];
					                       diff.abs();
					                       auto value = diff.sum();
					                       weight_diff_sums[i] = weight_diff_sums[i] + value;
				                       }
			                       }, this->node_vector_container->size()-1, this->node_vector_container->data());
			
			*model_weights_file << tick;
			for (int i = 0; i < number_of_layers; ++i)
			{
				*model_weights_file << "," << weight_diff_sums[i];
			}
			*model_weights_file << std::endl;
			
			delete[] weight_diff_sums;
		}
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> destruction_service() override
	{
		model_weights_file->flush();
		model_weights_file->close();
		
		return {record_service_status::success, ""};
	}

private:
	std::shared_ptr<std::ofstream> model_weights_file;
};

template <typename model_datatype>
class force_broadcast_model : public service<model_datatype>
{
public:
	int tick_to_broadcast;
	
	force_broadcast_model()
	{
		tick_to_broadcast = 0;
	}
	
	std::tuple<record_service_status, std::string> apply_config(const configuration_file::json& config) override
	{
		this->enable = config["enable"];
		this->tick_to_broadcast = config["broadcast_interval"];
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *>& _node_container, std::vector<node<model_datatype>*>& _node_vector_container) override
	{
		this->set_node_container(_node_container, _node_vector_container);
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> process_per_tick(int tick) override
	{
		if (this->enable == false) return {record_service_status::skipped, "not enabled"};
		
		auto model_sum = (*this->node_vector_container)[0]->solver->get_parameter();
		model_sum.set_all(0);
		if (tick % tick_to_broadcast == 0 && tick != 0)
		{
			LOG(INFO) << "force_broadcast_model triggered at tick: " << tick;
			for (auto& node: *(this->node_container))
			{
				model_sum = model_sum + node.second->solver->get_parameter();
			}
			model_sum = model_sum / this->node_container->size();
			
			for (auto& node: *(this->node_container))
			{
				node.second->solver->set_parameter(model_sum);
			}
		}
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> destruction_service() override
	{
		return {record_service_status::success, ""};
	}
};

template <typename model_datatype>
class peer_control_service : public service<model_datatype>
{
public:
	enum fedavg_buffer_size_strategy
	{
		unknown_strategy = 0,
		static_strategy,
		linear_strategy
	};
	
public:
	std::unordered_map<std::string, int> last_time_changed;
	std::unordered_map<std::string, int> as_peer_count;
	fedavg_buffer_size_strategy fedavg_buffer_strategy;
	int least_peer_change_interval;
	float accuracy_threshold_high;
	float accuracy_threshold_low;
	std::string ml_solver_proto;
	Ml::data_converter<model_datatype>* test_dataset;
	int ml_test_batch_size;
	std::vector<int>* ml_dataset_all_possible_labels;
	
	std::shared_ptr<std::ofstream> peer_change_file;
	
	peer_control_service()
	{
		fedavg_buffer_strategy = unknown_strategy;
		least_peer_change_interval = 0;
		accuracy_threshold_high = 0.0f;
		accuracy_threshold_low = 0.0f;
		
		size_t solver_for_testing_size = std::thread::hardware_concurrency();
		solver_for_testing = new Ml::MlCaffeModel<float, caffe::SGDSolver>[solver_for_testing_size];
	}
	
	std::tuple<record_service_status, std::string> apply_config(const configuration_file::json& config) override
	{
		this->enable = config["enable"];
		this->least_peer_change_interval = config["least_peer_change_interval"];
		accuracy_threshold_high = config["accuracy_threshold_high"];
		accuracy_threshold_low = config["accuracy_threshold_low"];
		LOG_IF(FATAL, accuracy_threshold_low >= accuracy_threshold_high) << "accuracy_threshold_low < accuracy_threshold_high must satisfy";
		
		std::string fedavg_buffer_strategy_str = config["fedavg_buffer_size"];
		if (fedavg_buffer_strategy_str == "static") fedavg_buffer_strategy = fedavg_buffer_size_strategy::static_strategy;
		else if (fedavg_buffer_strategy_str == "linear") fedavg_buffer_strategy = fedavg_buffer_size_strategy::linear_strategy;
		else LOG(FATAL) << "unknown fedavg_buffer_strategy in node";
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *>& _node_container, std::vector<node<model_datatype>*>& _node_vector_container) override
	{
		this->set_node_container(_node_container, _node_vector_container);
		
		if (this->enable == false)
		{
			//copy planned peers to peers
			for (auto& node: *(this->node_container))
			{
				node.second->peers = node.second->planned_peers;
			}
		}
		else
		{
			//do nothing, add peers in the future
			
			//solver for testing
			size_t solver_for_testing_size = std::thread::hardware_concurrency();
			for (int i = 0; i < solver_for_testing_size; ++i)
			{
				solver_for_testing[i].load_caffe_model(ml_solver_proto);
			}
			
			peer_change_file.reset(new std::ofstream(output_path / "peer_change_record.txt", std::ios::binary));
		}
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> process_per_tick(int tick) override
	{
		if (this->enable == false) return {record_service_status::skipped, "not enabled"};
		
		//calculate accuracy
		tmt::ParallelExecution_StepIncremental([&tick, this](uint32_t index, uint32_t thread_index, node<model_datatype> *single_node)
		                       {
			                       if (tick - single_node->last_measured_tick < least_peer_change_interval) return;
			                       auto[test_data, test_label] = get_dataset_by_node_type(*test_dataset, *single_node, ml_test_batch_size, *ml_dataset_all_possible_labels);
			                       auto model = single_node->solver->get_parameter();
			                       solver_for_testing[thread_index].set_parameter(model);
			                       auto accuracy = solver_for_testing[thread_index].evaluation(test_data, test_label);
			                       single_node->nets_accuracy_only_record.emplace(tick, accuracy);
			                       single_node->last_measured_accuracy = accuracy;
			                       single_node->last_measured_tick = tick;
		                       }, this->node_vector_container->size(), this->node_vector_container->data());
		
		//process peers
		for (auto node_pair: *(this->node_container))
		{
			node<model_datatype>* node_pointer = node_pair.second;
			
			//do nothing if it has changed peers recently
			if (tick - last_time_changed[node_pointer->name] < least_peer_change_interval) continue;
			
			if (node_pointer->last_measured_accuracy >= accuracy_threshold_high && node_pointer->peers.size() != node_pointer->planned_peers.size())
			{
				//try add a peer
				bool add = false;
				std::string new_peer_name;
				for (const auto& [name, single_planned_peer]: node_pointer->planned_peers)
				{
					if (node_pointer->peers.find(name) == node_pointer->peers.end())
					{
						node_pointer->peers.emplace(name, single_planned_peer);
						new_peer_name = name;

						add = true;
						break;
					}
				}
				LOG_IF(FATAL, add == false) << "DFL simulator logic error, no node added to the peers";
				
				//update the buffer size for new peer
				as_peer_count[new_peer_name]++;
				auto& new_peer = *(this->node_container->at(new_peer_name));
				new_peer.buffer_size = std::round((float (new_peer.planned_buffer_size) / new_peer.planned_peers.size()) * as_peer_count[new_peer_name]);
				
				std::stringstream ss;
				ss << "tick:" << tick << "    " << node_pointer->name << "(accuracy: " << node_pointer->last_measured_accuracy << ") add " << new_peer_name << "(buffer size:" << new_peer.buffer_size << ")";
				*peer_change_file << ss.str() << std::endl;
				LOG(INFO) << "[peer_control_service]  " << ss.str();
				last_time_changed[node_pointer->name] = tick;
			}
			
			if (node_pointer->last_measured_accuracy <= accuracy_threshold_low && node_pointer->peers.size() != 0)
			{
				//try delete a peer
				std::random_device rd;
				std::uniform_int_distribution<int> distribution(0, node_pointer->peers.size()-1);
				int delete_index = distribution(rd);
				std::string delete_peer_name;
				for (const auto& [name, single_peer]: node_pointer->peers)
				{
					if (delete_index == 0)
					{
						delete_peer_name = name;
						as_peer_count[delete_peer_name]--;
						auto& delete_peer = *(this->node_container->at(delete_peer_name));
						delete_peer.buffer_size = std::round((float (delete_peer.planned_buffer_size) / delete_peer.planned_peers.size()) * as_peer_count[delete_peer_name]);
						std::stringstream ss;
						ss << "tick:" << tick << "    " << node_pointer->name << "(accuracy: " << node_pointer->last_measured_accuracy << ") delete " << delete_peer_name << "(buffer size:" << delete_peer.buffer_size << ")";
						*peer_change_file << ss.str() << std::endl;
						LOG(INFO) << "[peer_control_service]  " << ss.str();
						break;
					}
					delete_index --;
				}
				node_pointer->peers.erase(delete_peer_name);
				last_time_changed[node_pointer->name] = tick;
			}
			
		}
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> destruction_service() override
	{
		if (peer_change_file && peer_change_file->is_open())
		{
			peer_change_file->flush();
			peer_change_file->close();
		}
		delete[] solver_for_testing;
		
		return {record_service_status::success, ""};
	}

private:
	Ml::MlCaffeModel<float, caffe::SGDSolver>* solver_for_testing;
};