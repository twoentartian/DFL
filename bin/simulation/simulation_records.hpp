#pragma once

#include <string>

#include "./node.hpp"

enum class record_service_status
{
	success,
	fail_not_specified_reason
	
};

template <typename model_datatype>
class record_service
{
public:
	record_service() = default;
	
	virtual std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *> node_container) = 0;
	
	virtual std::tuple<record_service_status, std::string> process_per_tick(int tick, std::unordered_map<std::string, node<model_datatype> *> node_container) = 0;
	
	virtual std::tuple<record_service_status, std::string> destruction_service() = 0;
};

template <typename model_datatype>
class accuracy_record : public record_service<model_datatype>
{
public:
	//set these variables before init
	std::string ml_solver_proto;
	int ml_test_interval_tick;
	Ml::data_converter<model_datatype>* test_dataset;
	int ml_test_batch_size;
	std::vector<node<model_datatype>*>* node_vector_container;
	
	std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *> node_container) override
	{
		LOG_IF(FATAL, test_dataset == nullptr) << "test_dataset is not set";
		LOG_IF(FATAL, node_vector_container == nullptr) << "node_vector_container is not set";
		
		accuracy_file.reset(new std::ofstream(output_path / "accuracy.csv", std::ios::binary));
		
		*accuracy_file << "tick";
		for (auto &single_node : node_container)
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
	
	std::tuple<record_service_status, std::string> process_per_tick(int tick, std::unordered_map<std::string, node<model_datatype> *> node_container) override
	{
		if (tick % ml_test_interval_tick == 0)
		{
			tmt::ParallelExecution([&tick, this](uint32_t index, uint32_t thread_index, node<model_datatype> *single_node)
			                       {
				                       auto[test_data, test_label] = test_dataset->get_random_data(ml_test_batch_size);
				                       auto model = single_node->solver->get_parameter();
				                       solver_for_testing[thread_index].set_parameter(model);
				                       auto accuracy = solver_for_testing[thread_index].evaluation(test_data, test_label);
				                       single_node->nets_accuracy_only_record.emplace(tick, accuracy);
			                       }, node_vector_container->size(), node_vector_container->data());

			//print accuracy to file
			*accuracy_file << tick;
			for (auto &single_node : node_container)
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
class model_weights_record : public record_service<model_datatype>
{
public:
	//set these variables before init
	std::vector<node<model_datatype>*>* node_vector_container;
	int ml_model_weight_diff_record_interval_tick;
	
	std::tuple<record_service_status, std::string> init_service(const std::filesystem::path& output_path, std::unordered_map<std::string, node<model_datatype> *> node_container) override
	{
		LOG_IF(FATAL, node_vector_container == nullptr) << "node_vector_container is not set";
		
		model_weights_file.reset(new std::ofstream(output_path / "model_weight_diff.csv", std::ios::binary));
		*model_weights_file << "tick";
		
		auto weights = (*node_vector_container)[0]->solver->get_parameter();
		auto layers = weights.getLayers();
		
		for (auto& single_layer: layers)
		{
			*model_weights_file << "," << single_layer.getName();
		}
		*model_weights_file << std::endl;
		
		return {record_service_status::success, ""};
	}
	
	std::tuple<record_service_status, std::string> process_per_tick(int tick, std::unordered_map<std::string, node<model_datatype> *> node_container) override
	{
		if (tick % ml_model_weight_diff_record_interval_tick == 0)
		{
			auto weights = (*node_vector_container)[0]->solver->get_parameter();
			auto layers = weights.getLayers();
			size_t number_of_layers = layers.size();
			auto* weight_diff_sums = new std::atomic<float>[number_of_layers];
			for (int i = 0; i < number_of_layers; ++i) weight_diff_sums[i] = 0;
			
			tmt::ParallelExecution([&tick, this, &weight_diff_sums,&number_of_layers](uint32_t index, uint32_t thread_index, node<model_datatype> *single_node)
			                       {
				                       uint32_t index_next = index + 1;
				                       const uint32_t total_size = node_vector_container->size();
				                       if (index_next == total_size-1) index_next = 0;
				                       auto net1 = (*node_vector_container)[index]->solver->get_parameter();
				                       auto net2 = (*node_vector_container)[index_next]->solver->get_parameter();
				                       auto layers1 = net1.getLayers();
				                       auto layers2 = net2.getLayers();
				                       for (int i = 0; i < number_of_layers; ++i)
				                       {
					                       auto diff = layers1[i] - layers2[i];
					                       diff.abs();
					                       auto value = diff.sum();
					                       weight_diff_sums[i] = weight_diff_sums[i] + value;
				                       }
			                       }, node_vector_container->size()-1, node_vector_container->data());
			
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
